#include "Text.h"

#include "mle/common/Assert.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Utils.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/Renderable.h"

namespace mle::ui::element {
namespace {
vk::DescriptorSetLayout dsl_sampler_;  // NOLINT
vk::DescriptorPool dpool_sampler_;     // NOLINT
vk::DescriptorSet dset_sampler_;       // NOLINT
vk::Sampler sampler_;                  // NOLINT

void initTempStuff() {
    vk::DescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = vk::DescriptorType::eSampler;
    binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    binding.descriptorCount = 1;
    vk::DescriptorSetLayoutCreateInfo dsl_ci;
    dsl_ci.bindingCount = 1;
    dsl_ci.setBindings(binding);
    dsl_sampler_ = renderer::unwrap(renderer::detail::getDevice().createDescriptorSetLayout(dsl_ci));

    vk::SamplerCreateInfo sampler_ci;
    sampler_ci.magFilter = vk::Filter::eLinear;
    sampler_ci.minFilter = vk::Filter::eLinear;
    sampler_ci.mipmapMode = vk::SamplerMipmapMode::eLinear;
    sampler_ci.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    sampler_ci.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    sampler_ci.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.maxAnisotropy = 1.0F;
    sampler_ci.borderColor = vk::BorderColor::eIntOpaqueBlack;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;
    sampler_ = renderer::unwrap(renderer::detail::getDevice().createSampler(sampler_ci));

    vk::DescriptorPoolSize sampler_pool_size;
    sampler_pool_size.type = vk::DescriptorType::eSampler;
    sampler_pool_size.descriptorCount = 1;

    vk::DescriptorPoolCreateInfo sampler_pool_ci;
    sampler_pool_ci.setPoolSizes(sampler_pool_size);
    sampler_pool_ci.maxSets = 1;
    dpool_sampler_ = renderer::unwrap(renderer::detail::getDevice().createDescriptorPool(sampler_pool_ci));

    vk::DescriptorSetAllocateInfo sampler_alloc_info;
    sampler_alloc_info.descriptorPool = dpool_sampler_;
    sampler_alloc_info.descriptorSetCount = 1;
    sampler_alloc_info.setSetLayouts(dsl_sampler_);
    dset_sampler_ = renderer::unwrap(renderer::detail::getDevice().allocateDescriptorSets(sampler_alloc_info)).at(0);

    vk::DescriptorImageInfo sampler_info;
    sampler_info.sampler = sampler_;
    sampler_info.imageView = nullptr;                        // Not needed for pure sampler
    sampler_info.imageLayout = vk::ImageLayout::eUndefined;  // Not needed for pure sampler

    vk::WriteDescriptorSet write_sampler;
    write_sampler.dstSet = dset_sampler_;
    write_sampler.dstBinding = 0;
    write_sampler.dstArrayElement = 0;
    write_sampler.descriptorType = vk::DescriptorType::eSampler;
    write_sampler.descriptorCount = 1;
    write_sampler.pImageInfo = &sampler_info;

    renderer::detail::getDevice().updateDescriptorSets(write_sampler, nullptr);

    renderer::addOnShutdown([]() {
        renderer::detail::getDevice().destroy(sampler_);
        renderer::detail::getDevice().destroy(dpool_sampler_);
        renderer::detail::getDevice().destroy(dsl_sampler_);
    });
}
}  // namespace

void Text::render(const RenderContext& ctx) const {
    struct Char {
        vec2f pos{};
        vec2f size{};
        vec4f color{};
        vec4f outline_color{};
        vec2f texture_offset{};
        vec2f texture_size{};
        u32 texture_idx{};
        f32 outline_thickness{};
    };

    std::vector<Char> chars;
    chars.reserve(render_text_.char_count);

    for (auto c : render_text_.tokens) {
        if (c.type != Font::RenderText::Token::Type::CHAR) {
            continue;
        }

        Char ch{};
        ch.size.x = c.rect.size.x / render_text_.width;
        ch.size.y = c.rect.size.y;

        ch.pos.x = c.rect.pos.x / render_text_.width;
        ch.pos.y = c.rect.pos.y;

        ch.color = color_;
        ch.texture_offset = c.texture_rect.pos;
        ch.texture_size = c.texture_rect.size;
        ch.texture_idx = c.texture_idx;

        ch.outline_color = outline_color_;
        ch.outline_thickness = outline_thickness_;

        // if (c.type == RenderText::Token::Type::BR) {
        //     ch.pos.x = element_pos.x;
        //     ch.pos.y += text_height_px_ / fbounds.size.y;
        // }

        chars.push_back(ch);
    }

    renderer::Buffer::CI instance_buffer_ci;
    instance_buffer_ci.size = sizeof(Char) * render_text_.char_count;
    instance_buffer_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    instance_buffer_ci.allocation_type = renderer::Buffer::CI::AllocationType::GPU_ONLY_HOST_WRITE_SEQ;
    auto instance_buffer = renderer::Buffer::createHnd(instance_buffer_ci);
    instance_buffer->update(chars.data());

    ctx.thread->setPipeline(getPipeline());
    ctx.thread->bindInstanceBuffer(instance_buffer.get());
    renderer::bindTexturesDSet(*ctx.thread);
    ctx.thread->bindDescriptorSet(dset_sampler_, 1);
    ctx.thread->draw(as<int>(render_text_.char_count), 4);

    renderer::deleteAfterFrame(std::move(instance_buffer));
}

void Text::setFont(const std::string& font_name) {
    font_ = ui::getFont(font_name);
}

void Text::setHeightPx(u32 height_px) {
    text_height_px_ = height_px;
}

void Text::setJustify(Justify justify) {
    justify_ = justify;
}

void Text::setWrap(bool wrap) {
    wrap_ = wrap;
}

void Text::setColor(Color color) {
    color_ = color;
}

void Text::setText(std::string text) {
    text_ = std::move(text);
    if (text_.empty()) {
        text_ = "<>";
    }
}

void Text::updateText() {
    // TODO: update only if text or font changed
    if (!font_) {
        font_ = ui::getFont();
    }
    render_text_ = font_->makeText(text_);
}

void Text::apply(entt::entity /*self*/, const sol::object& o) {
    MLE_ASSERT(o.valid());
    if (o.is<std::string>()) {
        auto str = lua::as<std::string>(o);
        setText(str);
    } else if (o.is<sol::table>()) {
        auto table = o.as<sol::table>();
        std::string text_string;
        auto lua_get_result = lua::tryGetKeyOrIdx(table, "text", 1, text_string);
        if (lua_get_result) {
            setText(text_string);
        }

        if (const sol::object color_r = table["color"]; color_r.valid()) {
            setColor(Color::fromLua(color_r));
        }

        if (const sol::object font_r = table["font"]; font_r.valid()) {
            MLE_ASSERT(font_r.is<std::string>());
            setFont(font_r.as<std::string>());
        } else if (!font_) {
            setFont();
        }

        if (const sol::object height_r = table["height"]; height_r.valid()) {
            MLE_ASSERT(height_r.get_type() == sol::type::number);
            setHeightPx(height_r.as<u32>());
        }

        if (const sol::object justify_r = table["justify"]; justify_r.valid()) {
            MLE_ASSERT(justify_r.is<std::string>());
            std::string justify_str = justify_r.as<std::string>();
            if (justify_str == "l") {
                setJustify(Text::Justify::LEFT);
            } else if (justify_str == "r") {
                setJustify(Text::Justify::RIGHT);
            } else if (justify_str == "j") {
                setJustify(Text::Justify::CENTER);
            } else {
                setJustify(Text::Justify::JUSTIFY);
            }
        }

        if (const sol::object outline_r = table["outline"]; outline_r.valid()) {
            auto str = outline_r.as<std::string>();
            auto colon_pos = str.find(':');
            if (colon_pos != std::string::npos) {
                outline_color_ = Color::fromString(str.substr(0, colon_pos));
                outline_thickness_ = std::stof(str.substr(colon_pos + 1));
            } else {
                outline_color_ = Color::fromString(str);
            }
        }

        if (const sol::object outline_color_r = table["outline_color"]; outline_color_r.valid()) {
            outline_color_ = lua::as<Color>(outline_color_r);
        }

        if (const sol::object outline_thickness_r = table["outline_thickness"]; outline_thickness_r.valid()) {
            outline_thickness_ = lua::as<f32>(outline_thickness_r);
        }

        if (const sol::object wrap_r = table["wrap"]; wrap_r.valid()) {
            MLE_ASSERT(wrap_r.is<bool>());
            setWrap(wrap_r.as<bool>());
        }

    } else {
        MLE_UNREACHABLE_LOG("Unexpected object type for Text: {}", o.get_type());
    }

    updateText();
}

renderer::PipelineRef Text::getPipeline() {
    static renderer::PipelineHnd pipeline;
    if (!pipeline) {
        initTempStuff();

        renderer::Pipeline::CI ci;
        ci.vertex_shader = renderer::getShader("mle/ui/sdf_text_instanced.vert");
        ci.fragment_shader = renderer::getShader("mle/ui/sdf_text_instanced.frag");
        ci.color_attachment_formats = {renderer::getDefaultColorFormat()};
        ci.blend_attachments = renderer::makeDefaultBlendAttachmentStates(1);
        ci.topology = vk::PrimitiveTopology::eTriangleStrip;
        ci.descriptor_set_layouts.emplace_back(renderer::getTexturesDescriptorSetLayout());
        ci.descriptor_set_layouts.emplace_back(dsl_sampler_);
        pipeline = renderer::Pipeline::createHnd(ci);
        renderer::addOnShutdown([&]() { pipeline.reset(); });
    }
    return pipeline.get();
}

vec2i Text::getSize() const {
    if (wrap_) {
        MLE_TODO;
    }

    return {render_text_.width * font_->getHeight(), font_->getHeight()};
}
}  // namespace mle::ui::element
