#include "Sprite.h"

#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "glm/gtc/constants.hpp"
#include "mle/lua/Types.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Utils.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "sol/forward.hpp"

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

void Sprite::renderComp(const RenderContext& ctx) const {
    struct {
        vec4f color{0};
        u32 tex_idx{};
    } pc;
    pc.color = color_;
    pc.tex_idx = texture_.idx;

    ctx.thread->setPipeline(getPipeline());
    renderer::bindTexturesDSet(*ctx.thread);
    ctx.thread->bindDescriptorSet(dset_sampler_, 1);
    ctx.thread->pushConstants(&pc);
    ctx.thread->draw(1, 4);
}

void Sprite::setTexture(entt::entity self, const std::string& texture_name) {
    texture_ = renderer::getTexture(texture_name);

    auto& renderable = getRegistry().get<comp::Renderable>(self);
    renderable.aspect_ratio = texture_.image->getAspectRatio();
}

void Sprite::setColor(const sol::object& o) {
    setColor(Color::fromLua(o));
}

void Sprite::setColor(Color color) {
    color_ = color;
}

void Sprite::lkh(entt::entity self, const sol::object& o) {
    MLE_ASSERT(o.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<Sprite>(self);

    if (!comp) {
        comp = &reg.emplace<Sprite>(self);
    }

    comp::Renderable::add(self, [](entt::entity self) -> RenderableInterface& { return getRegistry().get<Sprite>(self); });

    if (o.is<std::string>()) {
        comp->setTexture(self, o.as<std::string>());
    } else if (o.is<sol::table>()) {
        auto table = o.as<sol::table>();
        std::string texture_name;
        auto lua_get_result = lua::tryGetKeyOrIdx(table, "texture", 1, texture_name);
        MLE_ASSERT(lua_get_result);
        comp->setTexture(self, texture_name);

        if (const sol::object color_r = table["color"]; color_r.valid()) {
            comp->setColor(color_r);
        }
    } else {
        MLE_UNREACHABLE_LOG("Unexpected object type for Sprite: {}", o.get_type());
    }
}

renderer::PipelineRef Sprite::getPipeline() {
    static renderer::PipelineHnd pipeline;
    if (!pipeline) {
        initTempStuff();

        renderer::Pipeline::CI ci;
        ci.vertex_shader = renderer::getShader("ui/sprite.vert", true);
        ci.fragment_shader = renderer::getShader("ui/sprite.frag", true);
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
