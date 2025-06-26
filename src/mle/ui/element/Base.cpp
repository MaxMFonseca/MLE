#include "Base.h"

#include "Container.h"
#include "mle/common/Assert.h"
#include "mle/common/Utils.h"
#include "mle/lua/Lua.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Utils.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/Bounds.h"
#include "mle/ui/element/LuaKeyHandlers.h"
#include "mle/ui/element/Renderable.h"
#include "sol/forward.hpp"

namespace mle::ui::element {  // NOLINT
namespace comp {
void RootImage::apply(entt::entity /*unused*/, const sol::object& o) {
    auto table = lua::as<sol::table>(o);

    renderer::Image::CI ci;
    ci.usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment;
    ci.extent = lua::as<vec2i>(table["extent"]);
    ci.format = renderer::getDefaultColorFormat();
    image_handle = renderer::Image::createHnd(ci);
    clear_color = Color::fromLua(table["clear_color"]);
}

Container& Parent::container(entt::entity self) {
    auto& reg = getRegistry();
    return reg.get<Container>(reg.get<Parent>(self).parent);
}

renderer::PipelineRef Background::getPipeline() {
    static renderer::PipelineHnd pipeline;
    if (!pipeline) {
        renderer::Pipeline::CI ci;
        ci.vertex_shader = renderer::getShader("mle/ui/rect.vert");
        ci.fragment_shader = renderer::getShader("mle/ui/rect.frag");
        ci.color_attachment_formats = {renderer::getDefaultColorFormat()};
        ci.blend_attachments = renderer::makeDefaultBlendAttachmentStates(1);
        ci.topology = vk::PrimitiveTopology::eTriangleStrip;
        pipeline = renderer::Pipeline::createHnd(ci);
        renderer::addOnShutdown([&]() { pipeline.reset(); });
    }
    return pipeline.get();
}

void Background::render(const RenderContext& ctx) const {
    ctx.thread->setPipeline(Background::getPipeline());

    struct {
        vec4f color{};
    } pc;

    pc.color = color;

    ctx.thread->pushConstants(&pc);
    ctx.thread->draw(1, 4);
}

void Blur::render(const RenderContext& ctx) {
    ctx.thread->endRendering();

    if (!image || image->getExtent() != vec2i(ctx.thread->getViewport().size)) {
        renderer::Image::CI image_ci;
        image_ci.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
        image_ci.extent = vec2i(ctx.thread->getViewport().size);
        image_ci.format = renderer::getDefaultColorFormat();
        image = renderer::Image::createHnd(image_ci);
    }

    image->updateCopy(ctx.thread->cmd(), ctx.thread->getColor0Image(), ctx.thread->getViewport().asI32());

    image->transitionState(ctx.thread->cmd(), renderer::Image::State::SHADER_READ);

    ctx.thread->beginRendering({}, false);

    vk::DescriptorImageInfo image_info;
    image_info.imageLayout = image->getCurrentLayout();
    image_info.imageView = image->getDefaultView();
    vk::WriteDescriptorSet write0;
    write0.descriptorCount = 1;
    write0.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    write0.dstBinding = 0;
    write0.setImageInfo(image_info);

    ctx.thread->setPipeline(getPipeline().first);

    ctx.thread->pushDescriptor(0, {write0});

    ctx.thread->setViewport(ctx.thread->getViewport());

    ctx.thread->draw(1, 4);
}

std::pair<renderer::PipelineRef, vk::DescriptorSetLayout> Blur::getPipeline() {
    static renderer::PipelineHnd pipeline;
    static vk::DescriptorSetLayout dsl;
    static vk::Sampler sampler;

    if (!pipeline) {
        MLE_T("Creating blur pipeline");

        MLE_T("  sampler");
        vk::SamplerCreateInfo sampler_ci;
        sampler_ci.magFilter = vk::Filter::eLinear;
        sampler_ci.minFilter = vk::Filter::eLinear;
        sampler_ci.addressModeU = vk::SamplerAddressMode::eMirroredRepeat;
        sampler_ci.addressModeV = vk::SamplerAddressMode::eMirroredRepeat;
        sampler_ci.addressModeW = vk::SamplerAddressMode::eMirroredRepeat;
        sampler_ci.anisotropyEnable = VK_FALSE;
        sampler_ci.maxAnisotropy = 1.0F;
        sampler_ci.borderColor = vk::BorderColor::eFloatOpaqueWhite;
        sampler_ci.unnormalizedCoordinates = VK_FALSE;
        sampler_ci.compareEnable = VK_FALSE;
        sampler_ci.compareOp = vk::CompareOp::eAlways;
        sampler_ci.mipmapMode = vk::SamplerMipmapMode::eLinear;
        sampler_ci.minLod = 0.0F;
        sampler_ci.maxLod = 0.0F;
        sampler_ci.mipLodBias = 0.0F;

        // TODO: move this out
        sampler = renderer::unwrap(renderer::detail::getDevice().createSampler(sampler_ci));

        MLE_T("  descriptor set layout");
        vk::DescriptorSetLayoutBinding binding;
        binding.binding = 0;
        binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        binding.descriptorCount = 1;
        binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        binding.pImmutableSamplers = &sampler;

        vk::DescriptorSetLayoutCreateInfo dsl_ci;
        dsl_ci.bindingCount = 1;
        dsl_ci.flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR;
        dsl_ci.pBindings = &binding;

        dsl = renderer::unwrap(renderer::detail::getDevice().createDescriptorSetLayout(dsl_ci));

        MLE_T("  pipeline");
        renderer::Pipeline::CI ci;
        ci.vertex_shader = renderer::getShader("mle/ui/quad.vert");
        ci.fragment_shader = renderer::getShader("mle/ui/blur.frag");
        ci.color_attachment_formats = {renderer::getDefaultColorFormat()};
        ci.blend_attachments = renderer::makeDefaultBlendAttachmentStates(1);
        ci.topology = vk::PrimitiveTopology::eTriangleStrip;
        ci.descriptor_set_layouts.emplace_back(dsl);
        pipeline = renderer::Pipeline::createHnd(ci);

        renderer::addOnShutdown([&]() {
            pipeline.reset();
            renderer::destroy(dsl);
            renderer::destroy(sampler);
        });
    }

    return std::make_pair(pipeline.get(), dsl);
}

void Table::apply(entt::entity /*unused*/, const sol::table& tbl) {
    if (!v.valid()) {
        v = lua::createTable(tbl, true);
    } else {
        lua::merge(v, tbl);
    }
}
}  // namespace comp
}  // namespace mle::ui::element
