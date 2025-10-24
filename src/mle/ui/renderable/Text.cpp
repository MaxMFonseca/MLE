#include "Text.h"

#include "mle/lua/Utils.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/Entt.h"
#include "mle/ui/Types.h"
#include "mle/ui/components/Renderable.h"
#include "mle/utils/String.h"

namespace mle::ui::renderable {
namespace {

auto getPipeline() {
    static const Pipeline* pipeline = nullptr;
    if (pipeline == nullptr) {
        Pipeline::CI pipeline_ci{};
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/ui/text.vert");
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/ui/text.frag");
        std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::COLOR)};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
        pipeline_ci.blend_attachments = blend_attachments;
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleStrip;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        pipeline_ci.push_descriptor = 0;

        pipeline = &Renderer::i().pipelineCache().setPipeline("mle_ui_text", pipeline_ci);
    }
    return pipeline;
}
}  // namespace

void Text::apply(const Entt& e, const sol::object& obj) {
    if (!obj.valid()) {
        MLE_E("Invalid object provided to Text::apply for entt {}", e.e());
        return;
    }
    auto* renderable = e.tryGet<comp::Renderable>();
    Text* self_p = nullptr;
    if (!renderable) {
        renderable = &e.emplace<comp::Renderable>(std::make_unique<Text>());
        self_p = as<Text*>(renderable->impl.get());
        renderable->packet_buffers_.at(0) = std::make_shared<TextPacket>();
        renderable->packet_buffers_.at(1) = std::make_shared<TextPacket>();
        renderable->packet_buffers_.at(2) = std::make_shared<TextPacket>();
    } else {
        MLE_ASSERT(renderable->impl);
        if (renderable->impl->getType() == Text::type()) {
            MLE_E("Renderable::apply called on entt {} with incompatible Renderable type. {}x{}", e.fullName(), renderable->impl->getType(), Text::type());
            return;
        }
        self_p = as<Text*>(renderable->impl.get());
    }
    self_p->set(obj);
}

void Text::setText(std::u32string src) {
    text = std::move(src);
}

void Text::setText(std::string_view src) {
    setText(toUtf32(src));
}

void Text::setColor(const Color& c) {
    versionUp();
    color = c;
}

void Text::setJustifyMode(std::string_view mode_str) {
    if (matchAny(mode_str, "start", "left")) {
        justify_mode = Font::JustifyMode::START;
    } else if (matchAny(mode_str, "center", "middle")) {
        justify_mode = Font::JustifyMode::CENTER;
    } else if (matchAny(mode_str, "end", "right")) {
        justify_mode = Font::JustifyMode::END;
    } else if (matchAny(mode_str, "space_between", "justify")) {
        justify_mode = Font::JustifyMode::SPACE_BETWEEN;
    } else {
        MLE_W("Unknown justify mode string provided to Text::setJustifyMode: {}", mode_str);
        justify_mode = Font::JustifyMode::START;
    }
};

void Text::set(const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<std::string>()) {
        auto text_src = obj.as<std::string>();
        setText(toUtf32(text_src));
        return;
    }

    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        if (const auto text_r = lua::getFirstKey(table, "text", 1); lua::valid<std::string>(text_r)) {
            setText(text_r.as<std::string>());
        }
        if (const sol::object color_r = table["color"]; color_r.valid()) {
            setColor(color_r);
        }
        if (const auto font_r = table["font"]; lua::valid<std::string>(font_r)) {
            setFont(lua::as<std::string>(font_r).c_str());
        }
        if (const auto font_height_r = table["font_height"]; font_height_r.valid()) {
            font_height_tb.set(font_height_r);
        }
        if (const auto font_justify_r = table["justify_mode"]; lua::valid<std::string>(font_justify_r)) {
            setJustifyMode(lua::as<std::string>(font_justify_r));
        }
        if (const auto font_wrap_r = table["wrap"]; lua::valid<bool>(font_wrap_r)) {
            wrap = lua::as<bool>(font_wrap_r);
        }
        if (const auto line_max_aspect_r = table["line_max_aspect"]; lua::valid<f32>(line_max_aspect_r)) {
            line_max_aspect = lua::as<f32>(line_max_aspect_r);
        }
        return;
    }

    MLE_W("Unsupported object type provided to Text::set");
}

vec2u Text::calculateBounds(const Entt& e, vec2u max_size) {
    FontRef font = Renderer::i().fontCache().get(font_id);

    const int font_height_px = [&]() {
        switch (font_height_tb.type) {
            case TargetBound::Type::PX:
                return as<int>(font_height_tb.val);
            case TargetBound::Type::ROOT:
                return as<int>(as<f32>(e.ui().getRootSize().y) * font_height_tb.val);
            case TargetBound::Type::DEFAULT:
            case TargetBound::Type::FLEX_SHARE:
            case TargetBound::Type::RELATIVE:
            case TargetBound::Type::RELATIVE_H:
                return as<int>(as<f32>(max_size.y) * (font_height_tb.val == 0 ? 1.0F : font_height_tb.val));
            case TargetBound::Type::RELATIVE_W:
                return as<int>(as<f32>(max_size.x) * font_height_tb.val);
            case TargetBound::Type::FIT:
                if (line_max_aspect <= 0.F) {
                    // NOLINTNEXTLINE(bugprone-lambda-function-name) Dont care
                    MLE_W("Text::calculateBounds: line_max_aspect must be set to a positive value when using FIT font height.");
                    return as<int>(max_size.y);
                }
                return as<int>(as<f32>(max_size.x) / line_max_aspect);
        }
    }();

    font_height_px_f = as<f32>(font_height_px);
    if (line_max_aspect <= 0.F) {
        line_max_aspect = as<f32>(max_size.x) / font_height_px_f;
    }

    Font::MakeTextIn make_text_in{};
    make_text_in.str = text;
    make_text_in.justify_mode = justify_mode;
    make_text_in.wrap = wrap;
    make_text_in.line_max_aspect = as<f32>(max_size.x) / as<f32>(font_height_px);

    render_text = font->makeText(make_text_in);

    rendered_text_px.x = as<u32>(render_text.line_max_width * as<f32>(font_height_px));
    rendered_text_px.y = as<u32>(as<f32>(font_height_px) * as<f32>(render_text.line_count));

    vec2u final_size = glm::min(rendered_text_px, max_size);

    makeCharsBuffer(final_size);

    versionUp();

    return final_size;
}

void Text::makeCharsBuffer(vec2u element_size) {
    auto& font = *Renderer::i().fontCache().get(font_id);
    std::map<ImageRef, std::vector<TextPacket::CharsInstance>> image_to_chars_map;
    auto rendered_size_f = as<vec2f>(rendered_text_px);
    auto element_size_f = as<vec2f>(element_size);
    auto size_scale_f = glm::min(element_size_f / rendered_size_f, vec2f{1.0F});
    for (auto c : render_text.chars) {
        std::pair<ImageRef, TextureAtlas::Entry> atlas_entry = font.getTextureEntry(c.codepoint);
        auto& new_char = image_to_chars_map[atlas_entry.first].emplace_back();
        new_char.pos = c.rect.pos() * font_height_px_f;
        new_char.size = c.rect.size() * font_height_px_f;
        new_char.pos /= rendered_size_f;
        new_char.size /= rendered_size_f;
        new_char.pos *= size_scale_f;
        new_char.size *= size_scale_f;
        new_char.texture_pos = atlas_entry.second.pos();
        new_char.texture_size = atlas_entry.second.size();
    }
    current_rendering_chars_instance_data.clear();
    for (const auto& [image_ref, chars] : image_to_chars_map) {
        per_image_data.push_back({image_ref, current_rendering_chars_instance_data.size(), chars.size()});
        current_rendering_chars_instance_data.insert(current_rendering_chars_instance_data.end(), chars.begin(), chars.end());
    }
}

void Text::doUpdatePacket(RenderablePacketI* packet) {
    auto& p = *as<TextPacket*>(packet);

    if (p.chars_buffer) {
        Renderer::i().frameRenderer().deleteAfterFrame(std::move(p.chars_buffer));
    }
    Buffer::CI buffer_ci{};
    buffer_ci.size = as<usize>(current_rendering_chars_instance_data.size() * sizeof(TextPacket::CharsInstance));
    buffer_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    buffer_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY_HOST_WRITE_SEQ;
    p.chars_buffer = Buffer::createHnd(buffer_ci);
    p.chars_buffer->write(current_rendering_chars_instance_data.data());
    p.per_image_data = per_image_data;
    p.color = color;
};

void TextPacket::render(CompRenderingCtx& ctx) {
    auto& thread = ctx.thread;

    const auto* pipeline = getPipeline();

    thread.setPipeline(pipeline);

    thread.bindVertexBuffer(chars_buffer.get());

    for (auto& [image, instance_offset, instance_count] : per_image_data) {
        vk::DescriptorImageInfo b0_0_di = image->getDescriptorInfo();
        auto push_writes = pipeline->makeWrites(0, nullptr, &b0_0_di);

        thread.pushDescriptor(0, push_writes);

        // struct {
        //     vec4f color;
        //     vec4i rounding_corners_radius_px;
        //     vec2i viewport_size;
        // } pc{};
        //
        // pc.color = color;
        // pc.viewport_size = ctx.viewport_size;
        // pc.rounding_corners_radius_px = ctx.rounding_corners_radius_px;
        //
        // thread.pushConstants(&pc);

        thread.draw(4, instance_count, 0, instance_offset);
    }
}

}  // namespace mle::ui::renderable
