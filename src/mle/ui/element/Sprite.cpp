#include "Sprite.h"

#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

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
vk::DescriptorSetLayout dsl_, dsl_sampler_;  // NOLINT
vk::DescriptorPool dpool_;                   // NOLINT
vk::DescriptorSet dset_;                     // NOLINT

void initTempStuff() {
    {
        vk::DescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = vk::DescriptorType::eSampledImage;
        binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        binding.descriptorCount = 500000;
        vk::DescriptorSetLayoutCreateInfo dsl_ci;
        dsl_ci.bindingCount = 1;
        dsl_ci.setBindings(binding);
        dsl_ci.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;

        vk::DescriptorBindingFlags flags = vk::DescriptorBindingFlagBits::eVariableDescriptorCount | vk::DescriptorBindingFlagBits::eUpdateAfterBind |
                                           vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;
        vk::DescriptorSetLayoutBindingFlagsCreateInfo binding_flags_ci;
        binding_flags_ci.bindingCount = 1;
        binding_flags_ci.pBindingFlags = &flags;
        dsl_ci.pNext = &binding_flags_ci;

        dsl_ = renderer::unwrap(renderer::detail::getDevice().createDescriptorSetLayout(dsl_ci));
    }

    {
        vk::DescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = vk::DescriptorType::eSampler;
        binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        binding.descriptorCount = 1;
        vk::DescriptorSetLayoutCreateInfo dsl_ci;
        dsl_ci.bindingCount = 1;
        dsl_ci.setBindings(binding);
        dsl_sampler_ = renderer::unwrap(renderer::detail::getDevice().createDescriptorSetLayout(dsl_ci));
    }

    vk::DescriptorPoolSize pool_size;
    pool_size.type = vk::DescriptorType::eSampledImage;
    pool_size.descriptorCount = 1024;

    vk::DescriptorPoolCreateInfo pool_ci;
    pool_ci.setPoolSizes(pool_size);
    pool_ci.maxSets = 2;
    pool_ci.flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;

    dpool_ = renderer::unwrap(renderer::detail::getDevice().createDescriptorPool(pool_ci));

    vk::DescriptorSetVariableDescriptorCountAllocateInfo variable_info;
    variable_info.descriptorSetCount = 1;
    variable_info.pDescriptorCounts = &pool_size.descriptorCount;

    vk::DescriptorSetAllocateInfo alloc_info;
    alloc_info.pNext = &variable_info;
    alloc_info.descriptorPool = dpool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.setSetLayouts(dsl_);

    dset_ = renderer::unwrap(renderer::detail::getDevice().allocateDescriptorSets(alloc_info)).at(0);
}
}  // namespace

void Sprite::renderComp(entt::entity /*self*/, renderer::RenderingThreadRef /*thread*/) const {
    MLE_VC(1);
    // auto& reg = getRegistry();
    //
    // thread->setPipeline(getPipeline());
    // struct {
    //     vec2f pos{};
    //     vec2f size{};
    //     vec4f color{};
    // } pc;
    // // FIXME: the context is wrong here, this should be renderd on the current root
    // // the thread does have the context for size, but not pos/rotation
    // // also we need to find a way to do clipping
    // pc.pos = fcbounds.pos / fbounds.size;
    // pc.size = fcbounds.size / fbounds.size;
    // pc.color = bg->color;
    // thread->pushConstants(&pc);
    // thread->setViewport();
    // thread->draw(1, 4);
}

void Sprite::setTexture(const std::string& texture_name) {
    texture_ = renderer::getTexture(texture_name);
    getPipeline();
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
        comp->setTexture(o.as<std::string>());
    } else if (o.is<sol::table>()) {
        auto table = o.as<sol::table>();
        std::string texture_name;
        auto lua_get_result = lua::tryGetKeyOrIdx(table, "texture", 1, texture_name);
        MLE_ASSERT(lua_get_result);
        comp->setTexture(texture_name);

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
        ci.descriptor_set_layouts.emplace_back(dsl_);
        ci.descriptor_set_layouts.emplace_back(dsl_sampler_);
        pipeline = renderer::Pipeline::createHnd(ci);
        renderer::addOnShutdown([&]() { pipeline.reset(); });
    }
    return pipeline.get();
}
}  // namespace mle::ui::element::comp
