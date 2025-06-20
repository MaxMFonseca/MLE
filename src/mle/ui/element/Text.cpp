#include "Text.h"

#include "mle/common/Assert.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Utils.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/Renderable.h"

namespace mle::ui::element::comp {
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

void Text::renderComp(const RenderContext& ctx) const {
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
        ch.size.x = (as<f32>(c.rect.size.x) / as<f32>(render_text_.width));
        ch.size.y = (as<f32>(c.rect.size.y) / as<f32>(font_->getHeight()));

        ch.pos.x = (as<f32>(c.rect.pos.x) / as<f32>(render_text_.width));
        ch.pos.y = (as<f32>(c.rect.pos.y) / as<f32>(font_->getHeight()));

        ch.color = color_;
        ch.texture_offset = c.texture_rect.pos;
        ch.texture_size = c.texture_rect.size;
        ch.texture_idx = 0;

        // ch.outline_color = color_;
        // ch.outline_thickness = 0.0F;  // No outline for now

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

void Text::updateText(entt::entity self) {
    render_text_ = font_->makeText(text_);
    auto& renderable = getRegistry().get<comp::Renderable>(self);
    renderable.aspect_ratio = static_cast<f32>(render_text_.width) / static_cast<f32>(font_->getHeight());
}

void Text::lkh(entt::entity self, const sol::object& o) {
    MLE_ASSERT(o.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<Text>(self);

    if (!comp) {
        comp = &reg.emplace<Text>(self);
    }

    comp::Renderable::add(self, [](entt::entity self) -> RenderableInterface& { return getRegistry().get<Text>(self); });

    if (o.is<sol::table>()) {
        auto table = o.as<sol::table>();
        std::string text_string;
        auto lua_get_result = lua::tryGetKeyOrIdx(table, "texture", 1, text_string);
        MLE_ASSERT(lua_get_result);
        comp->setText(text_string);

        if (const sol::object color_r = table["color"]; color_r.valid()) {
            comp->setColor(Color::fromLua(color_r));
        }

        if (const sol::object font_r = table["font"]; font_r.valid()) {
            MLE_ASSERT(font_r.is<std::string>());
            comp->setFont(font_r.as<std::string>());
        } else if (!comp->font_) {
            comp->setFont();
        }

        if (const sol::object height_r = table["height"]; height_r.valid()) {
            MLE_ASSERT(height_r.get_type() == sol::type::number);
            comp->setHeightPx(height_r.as<u32>());
        }

        if (const sol::object justify_r = table["justify"]; justify_r.valid()) {
            MLE_ASSERT(justify_r.is<std::string>());
            std::string justify_str = justify_r.as<std::string>();
            if (justify_str == "l") {
                comp->setJustify(Text::Justify::LEFT);
            } else if (justify_str == "r") {
                comp->setJustify(Text::Justify::RIGHT);
            } else if (justify_str == "j") {
                comp->setJustify(Text::Justify::CENTER);
            } else {
                comp->setJustify(Text::Justify::JUSTIFY);
            }
        }

        if (const sol::object wrap_r = table["wrap"]; wrap_r.valid()) {
            MLE_ASSERT(wrap_r.is<bool>());
            comp->setWrap(wrap_r.as<bool>());
        }

    } else {
        MLE_UNREACHABLE_LOG("Unexpected object type for Sprite: {}", o.get_type());
    }

    comp->updateText(self);
}

renderer::PipelineRef Text::getPipeline() {
    static renderer::PipelineHnd pipeline;
    if (!pipeline) {
        initTempStuff();

        renderer::Pipeline::CI ci;
        ci.vertex_shader = renderer::getShader("ui/sdf_text_instanced.vert", true);
        ci.fragment_shader = renderer::getShader("ui/sdf_text_instanced.frag", true);
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
}  // namespace mle::ui::element::comp
