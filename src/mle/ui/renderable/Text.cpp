#include "Text.h"

#include "mle/lua/Utils.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/Entt.h"
#include "mle/ui/Types.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Renderable.h"
#include "mle/utils/String.h"
#include "mle/window/TextBox.h"
#include "mle/window/UserInputManager.h"
#include "sol/forward.hpp"

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

Text::~Text() = default;

Expected<std::reference_wrapper<Text>> Text::getFromEntt(const Entt& ew) {
    auto* renderable = ew.tryGet<comp::Renderable>();
    if (!renderable) {
        return std::unexpected(Result::NOT_FOUND);
    }
    MLE_ASSERT(renderable->impl);
    if (renderable->impl->getType() != Text::type()) {
        return std::unexpected(Result::INVALID_TYPE);
    }
    auto* text_comp = as<Text*>(renderable->impl.get());
    return *text_comp;
}

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
        if (renderable->impl->getType() != Text::type()) {
            MLE_E("Renderable::apply called on entt {} with incompatible Renderable type. {}x{}", e.fullName(), renderable->impl->getType(), Text::type());
            return;
        }
        self_p = as<Text*>(renderable->impl.get());
    }
    self_p->set(e, obj);
}

void Text::applyInputEnable(const Entt& e, const sol::object& obj) {
    auto* renderable = e.tryGet<comp::Renderable>();
    if (!renderable || renderable->impl->getType() != Text::type()) {
        MLE_E("Text::applyInputEnable called on entt {} without Text renderable.", e.fullName());
        return;
    }
    auto* text_comp = as<Text*>(renderable->impl.get());
    if (lua::valid<bool>(obj) && !obj.as<bool>()) {
        text_comp->disableInputBox();
    } else {
        text_comp->enableInputBox();
    }
}

void Text::applyInputDisable(const Entt& ew, const sol::object& /*obj*/) {
    auto* renderable = ew.tryGet<comp::Renderable>();
    if (!renderable || renderable->impl->getType() != Text::type()) {
        MLE_E("Text::applyInputDisable called on entt {} without Text renderable.", ew.fullName());
        return;
    }
    auto* text_comp = as<Text*>(renderable->impl.get());
    text_comp->disableInputBox();
}

void Text::applyInputClear(const Entt& ew, const sol::object& /*obj*/) {
    auto* renderable = ew.tryGet<comp::Renderable>();
    if (!renderable || renderable->impl->getType() != Text::type()) {
        MLE_E("Text::applyInputClear called on entt {} without Text renderable.", ew.fullName());
        return;
    }
    auto* text_comp = as<Text*>(renderable->impl.get());
    if (!text_comp->input_tb) {
        MLE_W("Text::applyInputClear called on entt {} without TextBox.", ew.fullName());
        return;
    }
    if (text_comp->input_tb->getText().empty()) {
        return;
    }
    text_comp->input_tb->setText(U"");
    ew.requestInternalBoundsUpdate();
}

void Text::setText(const Entt& ew, std::u32string src) {
    text = std::move(src);
    ew.requestInternalBoundsUpdate();
}

void Text::setText(const Entt& ew, std::string_view src) {
    setText(ew, toUtf32(src));
}

void Text::setFont(const Entt& ew, const char* cstr) {
    font_id = entt::hashed_string{cstr};
    ew.requestInternalBoundsUpdate();
}

void Text::setColor(const Color& c) {
    color = c.toLinear();
    versionUp();
}

void Text::setBorderColor(const Color& c) {
    border_color = c.toLinear();
    versionUp();
};

void Text::setBold(bool t) {
    versionUp();
    bold = t;
}
void Text::setBorderThickness(f32 t) {
    versionUp();
    border_thickness = t / 16;
}

void Text::setFontHeight(const Entt& ew, const sol::object& obj) {
    font_height_tb.set(obj);
    ew.requestInternalBoundsUpdate();
}

void Text::setJustifyMode(const Entt& ew, std::string_view mode_str) {
    if (matchAny(mode_str, "start", "left", "b", "l")) {
        justify_mode = Font::JustifyMode::START;
    } else if (matchAny(mode_str, "center", "middle", "c")) {
        justify_mode = Font::JustifyMode::CENTER;
    } else if (matchAny(mode_str, "end", "right", "e", "r")) {
        justify_mode = Font::JustifyMode::END;
    } else if (matchAny(mode_str, "space_between", "justify")) {
        justify_mode = Font::JustifyMode::SPACE_BETWEEN;
    } else {
        MLE_W("Unknown justify mode string provided to Text::setJustifyMode: {}", mode_str);
        justify_mode = Font::JustifyMode::START;
    }
    ew.requestInternalBoundsUpdate();
};

void Text::setLineMaxAspect(const Entt& ew, f32 v) {
    line_max_aspect = v;
    ew.requestInternalBoundsUpdate();
};

void Text::setWrap(const Entt& ew, bool w) {
    wrap = w;
    ew.requestInternalBoundsUpdate();
};

void Text::set(const Entt& ew, const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<std::string>()) {
        auto text_src = obj.as<std::string>();
        setText(ew, toUtf32(text_src));
        return;
    }

    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        if (const auto text_r = lua::getFirstKey(table, "text", 1); lua::valid<std::string>(text_r)) {
            setText(ew, text_r.as<std::string>());
        }
        if (const sol::object color_r = table["color"]; color_r.valid()) {
            setColor(color_r);
        }
        if (const sol::object border_color_r = table["border_color"]; border_color_r.valid()) {
            setBorderColor(border_color_r);
        }
        if (const auto border_thickness_r = table["border_thickness"]; lua::valid<f32>(border_thickness_r)) {
            setBorderThickness(lua::as<f32>(border_thickness_r));
        }
        if (const auto bold_r = table["bold"]; lua::valid<bool>(bold_r)) {
            setBold(lua::as<bool>(bold_r));
        }
        if (const auto font_r = table["font"]; lua::valid<std::string>(font_r)) {
            setFont(ew, lua::as<std::string>(font_r).c_str());
        }
        if (const sol::object font_height_r = table["height"]; font_height_r.valid()) {
            setFontHeight(ew, font_height_r);
        }
        if (const auto font_justify_r = table["justify"]; lua::valid<std::string>(font_justify_r)) {
            setJustifyMode(ew, lua::as<std::string>(font_justify_r));
        }
        if (const auto font_wrap_r = table["wrap"]; lua::valid<bool>(font_wrap_r)) {
            setWrap(ew, lua::as<bool>(font_wrap_r));
        }
        if (const auto line_max_aspect_r = table["line_max_aspect"]; lua::valid<f32>(line_max_aspect_r)) {
            setLineMaxAspect(ew, lua::as<f32>(line_max_aspect_r));
        }
        if (const sol::object input_box_r = table["input"]; input_box_r.valid()) {
            makeInputBox(ew, input_box_r);
        }
        return;
    }

    MLE_W("Unsupported object type provided to Text::set");
}

vec2u Text::calculateBounds(const Entt& e, vec2u max_size) {
    if (text.empty() && input_tb == nullptr) {
        return {0, 0};
    }

    std::u32string final_text;
    if (input_tb != nullptr && !input_tb->getText().empty()) {
        final_text = input_tb->getText();
    } else {
        final_text = text;
    }

    FontRef font = Renderer::i().fontCache().get(font_id);

    const int font_height_px = [&]() {
        switch (font_height_tb.type) {
            case TargetBound::Type::PX:
                return as<int>(font_height_tb.val);
            case TargetBound::Type::ROOT:
                return as<int>(as<f32>(e.ui().getRootMaxSize().y) * font_height_tb.val);
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
            default:
                MLE_W("Text::calculateBounds unhandled TargetBound::Type in font height calculation.");
                return as<int>(max_size.y);
        }
    }();

    font_height_px_f = as<f32>(font_height_px);
    if (line_max_aspect <= 0.F) {
        line_max_aspect = as<f32>(max_size.x) / font_height_px_f;
    }

    Font::MakeTextIn make_text_in{};
    make_text_in.str = final_text;
    make_text_in.justify_mode = justify_mode;
    make_text_in.wrap = wrap;
    make_text_in.line_max_aspect = as<f32>(max_size.x) / as<f32>(font_height_px);

    render_text = font->makeText(make_text_in);

    vec2u rendered_text_px = render_text.text_extent * as<f32>(font_height_px);

    vec2u final_size = glm::min(rendered_text_px, max_size);

    chars_buffer_needs_update = true;

    versionUp();

    return final_size;
}

void Text::makeCharsBuffer(vec2u viewport_size) {
    auto& font = *Renderer::i().fontCache().get(font_id);
    std::map<ImageRef, std::vector<TextPacket::CharsInstance>> image_to_chars_map;

    auto viewport_size_f = as<vec2f>(viewport_size);
    auto text_extent_f = as<vec2f>(render_text.text_extent) * font_height_px_f;
    auto scale = text_extent_f / viewport_size_f;

    for (auto c : render_text.chars) {
        if (c.codepoint == U' ') {
            continue;
        }
        std::pair<ImageRef, TextureAtlas::Entry> atlas_entry = font.getTextureEntry(c.codepoint);
        auto& new_char = image_to_chars_map[atlas_entry.first].emplace_back();
        new_char.texture_pos = atlas_entry.second.pos();
        new_char.texture_size = atlas_entry.second.size();

        new_char.pos = c.rect.pos() / render_text.text_extent * scale;
        new_char.size = c.rect.size() / render_text.text_extent * scale;
    }
    current_rendering_chars_instance_data.clear();
    per_image_data.clear();
    for (const auto& [image_ref, chars] : image_to_chars_map) {
        per_image_data.push_back({.image_ref = image_ref, .instance_offset = current_rendering_chars_instance_data.size(), .instance_count = chars.size()});
        current_rendering_chars_instance_data.insert(current_rendering_chars_instance_data.end(), chars.begin(), chars.end());
    }
    chars_buffer_needs_update = false;
}

void Text::doUpdatePacket(const Entt& ew, RenderablePacketI* packet) {
    if (text.empty()) {
        return;
    }

    if (chars_buffer_needs_update) {
        auto& bounds = ew.get<comp::Bounds>();
        makeCharsBuffer(bounds.parent_px.size());
    }

    auto& p = *as<TextPacket*>(packet);

    if (p.chars_buffer) {
        Renderer::i().frameRenderer().addToGC(std::move(p.chars_buffer));
    }
    Buffer::CI buffer_ci{};
    buffer_ci.size = as<usize>(current_rendering_chars_instance_data.size() * sizeof(TextPacket::CharsInstance));
    buffer_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    buffer_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY_HOST_WRITE_SEQ;
    p.chars_buffer = Buffer::createHnd(buffer_ci);
    p.chars_buffer->write(current_rendering_chars_instance_data.data());
    p.per_image_data = per_image_data;
    p.color = color;
    p.border_color = border_color;
    p.border_thickness = border_thickness;
    p.bold = bold;
};

void Text::makeInputBox(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(!input_tb);

    input_tb = std::make_unique<TextBox>();
    input_tb->setChangedCallback([e]() { e.requestInternalBoundsUpdate(); });

    bool multiline_input = false;
    bool ctrl_enter_newline = false;
    if (lua::valid<sol::table>(obj)) {
        auto table = obj.as<sol::table>();
        lua::tryGetKeyAs(table, "multiline", multiline_input);
        lua::tryGetKeyAs(table, "ctrl_enter_newline", ctrl_enter_newline);
    }

    if (multiline_input) {
        input_tb->setAllowNewLine(true);
        input_tb->setNewLineCtrlEnter(ctrl_enter_newline);
    }
};

void Text::enableInputBox() const {
    if (!input_tb) {
        MLE_W("Text::enableInputBox called but no input box found.");
        return;
    }
    input_tb->setFocused(true);
};

void Text::disableInputBox() const {
    input_tb->setFocused(false);
};

TextPacket::~TextPacket() {
    Renderer::i().frameRenderer().addToGC(std::move(chars_buffer));
};

void TextPacket::render(CompRenderingCtx& ctx) {
    if (!chars_buffer) {
        return;
    }

    auto& thread = ctx.thread;

    const auto* pipeline = getPipeline();

    thread.setPipeline(pipeline);

    thread.bindVertexBuffer(chars_buffer.get());

    for (auto& [image, instance_offset, instance_count] : per_image_data) {
        vk::DescriptorImageInfo b0_0_di = image->getDescriptorInfo();
        auto push_writes = pipeline->makeWrites(0, nullptr, &b0_0_di);

        thread.pushDescriptor(0, push_writes);

        struct PC {
            vec4f color;
            vec4f border_color;
            f32 border_thickness;
            f32 text_thickness;
        } pc{};
        pc.color = color;
        pc.border_color = border_color;
        pc.border_thickness = border_thickness;
        pc.text_thickness = 0.5F;
        if (bold) {
            pc.text_thickness += 0.5F / 8;
        }

        thread.pushConstants(&pc);

        thread.draw(4, instance_count, 0, instance_offset);
    }
}

}  // namespace mle::ui::renderable
