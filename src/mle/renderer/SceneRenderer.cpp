#include "SceneRenderer.h"

#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "mle/renderer/Image.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"

namespace mle::renderer {
void SceneRenderer::shutdown() {
    MLE_D("Shutting down scene renderer");

    detail::waitIdle();

    detail::getDevice().destroy(sampler_);
    detail::getDevice().destroy(lighting_dsl_);
    g_pipeline_.reset();
    plane_pipeline_.reset();
    lighting_pipeline_.reset();
    target_image_.reset();
    g_.albedo.reset();
    g_.normal.reset();
    g_.material.reset();
    g_.depth.reset();
}

void SceneRenderer::init(vec2i extent) {
    MLE_D("Initializing scene renderer with extent: {}", extent);

    camera_.setAspect(static_cast<f32>(extent.x) / static_cast<f32>(extent.y));
    camera_.setFov(60.0F);
    camera_.setNear(0.1F);
    camera_.setFar(1000.0F);

    MLE_D("Creating images");
    Image::CI image_ci{};
    image_ci.extent = extent;
    image_ci.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
    image_ci.format = vk::Format::eR8G8B8A8Unorm;
    g_.albedo = Image::createHnd(image_ci);
    g_.normal = Image::createHnd(image_ci);
    g_.material = Image::createHnd(image_ci);
    image_ci.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    image_ci.format = getDepthFormat();
    g_.depth = Image::createHnd(image_ci);

    image_ci.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
    image_ci.format = vk::Format::eR8G8B8A8Unorm;
    target_image_ = Image::createHnd(image_ci);

    MLE_D("Creating Pipelines");
    createGPipeline();
    // createPlanePipeline();
    createLightingPipeline();
    createWDS();

    MLE_D("Scene renderer initialized");
}

void SceneRenderer::createGPipeline() {
    MLE_D("Creating G-Buffer pipeline");

    Pipeline::CI pipeline_ci;
    pipeline_ci.vertex_shader = getShader("mle/scene/vox.vert");
    pipeline_ci.fragment_shader = getShader("mle/scene/vox.frag");
    pipeline_ci.depth = true;
    pipeline_ci.color_attachment_formats.emplace_back(vk::Format::eR8G8B8A8Unorm);
    pipeline_ci.color_attachment_formats.emplace_back(vk::Format::eR8G8B8A8Unorm);
    pipeline_ci.color_attachment_formats.emplace_back(vk::Format::eR8G8B8A8Unorm);
    pipeline_ci.blend_attachments = makeDefaultBlendAttachmentStates(3, false);
    pipeline_ci.topology = vk::PrimitiveTopology::eTriangleList;

    g_pipeline_ = Pipeline::createHnd(pipeline_ci);
}

void SceneRenderer::createLightingPipeline() {
    MLE_D("Creating lighting pipeline");

    vk::SamplerCreateInfo sampler_ci;

    sampler_ = unwrap(detail::getDevice().createSampler(sampler_ci));
    sampler_ci.magFilter = vk::Filter::eLinear;
    sampler_ci.minFilter = vk::Filter::eLinear;
    sampler_ci.mipmapMode = vk::SamplerMipmapMode::eLinear;
    sampler_ci.addressModeU = vk::SamplerAddressMode::eRepeat;
    sampler_ci.addressModeV = vk::SamplerAddressMode::eRepeat;
    sampler_ci.addressModeW = vk::SamplerAddressMode::eRepeat;
    sampler_ci.maxLod = VK_LOD_CLAMP_NONE;

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.resize(3);
    for (usize i = 0; i < bindings.size(); ++i) {
        bindings[i].setBinding(i);
        bindings[i].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
        bindings[i].setDescriptorCount(1);
        bindings[i].setStageFlags(vk::ShaderStageFlagBits::eFragment);
        bindings[i].setImmutableSamplers(sampler_);
    }

    vk::DescriptorSetLayoutCreateInfo dsl_ci;
    dsl_ci.setBindings(bindings);
    dsl_ci.setFlags(vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR);

    lighting_dsl_ = unwrap(detail::getDevice().createDescriptorSetLayout(dsl_ci));

    Pipeline::CI pipeline_ci;
    pipeline_ci.vertex_shader = getShader("mle/fs_triangle.vert");
    pipeline_ci.fragment_shader = getShader("mle/scene/light.frag");
    pipeline_ci.depth = true;
    pipeline_ci.color_attachment_formats.emplace_back(vk::Format::eR8G8B8A8Unorm);
    pipeline_ci.blend_attachments = makeDefaultBlendAttachmentStates(1);
    pipeline_ci.topology = vk::PrimitiveTopology::eTriangleList;
    pipeline_ci.descriptor_set_layouts.emplace_back(lighting_dsl_);

    lighting_pipeline_ = Pipeline::createHnd(pipeline_ci);
}

void SceneRenderer::createWDS() {
    MLE_D("Creating write descriptor sets for G-Buffer");

    auto& albedo_info = lighting_dii_[0];
    auto& normal_info = lighting_dii_[1];
    auto& material_info = lighting_dii_[2];

    albedo_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    albedo_info.imageView = g_.albedo->getView();
    normal_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    normal_info.imageView = g_.normal->getView();
    material_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    material_info.imageView = g_.material->getView();

    auto& b0 = lighting_wds_.emplace_back();
    b0.setImageInfo(lighting_dii_[0]);
    b0.setDstBinding(0);
    b0.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    auto& b1 = lighting_wds_.emplace_back();
    b1.setImageInfo(lighting_dii_[1]);
    b1.setDstBinding(1);
    b1.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    auto& b2 = lighting_wds_.emplace_back();
    b2.setImageInfo(lighting_dii_[2]);
    b2.setDstBinding(2);
    b2.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
}

void SceneRenderer::render() {
    RenderingThread thread;
    thread.init();

    g_.albedo->transitionState(thread.cmd(), Image::State::COLOR_ATT);
    g_.normal->transitionState(thread.cmd(), Image::State::COLOR_ATT);
    g_.material->transitionState(thread.cmd(), Image::State::COLOR_ATT);
    g_.depth->transitionState(thread.cmd(), Image::State::DEPTH_ATT);

    {
        std::vector<AttachmentInfo> attachments;
        auto& albedo_att = attachments.emplace_back();
        albedo_att.image = g_.albedo.get();
        albedo_att.load = vk::AttachmentLoadOp::eClear;
        albedo_att.store = vk::AttachmentStoreOp::eStore;
        albedo_att.clear_value.setColor({0.2F, 0.2F, 0.2F, 0.2F});
        auto& normal_att = attachments.emplace_back();
        normal_att.image = g_.normal.get();
        normal_att.load = vk::AttachmentLoadOp::eClear;
        normal_att.store = vk::AttachmentStoreOp::eStore;
        normal_att.clear_value.setColor({0.3F, 0.3, 0.3, 0.3});
        auto& material_att = attachments.emplace_back();
        material_att.image = g_.material.get();
        material_att.load = vk::AttachmentLoadOp::eClear;
        material_att.store = vk::AttachmentStoreOp::eStore;
        thread.setColorAttachments(std::move(attachments));
    }

    AttachmentInfo depth_attachment{};
    depth_attachment.image = g_.depth.get();
    depth_attachment.load = vk::AttachmentLoadOp::eClear;
    depth_attachment.store = vk::AttachmentStoreOp::eStore;
    depth_attachment.clear_value.setDepthStencil({1.0F, 0});
    thread.setDepthAttachment(depth_attachment);

    thread.beginRendering();
    thread.setPipeline(g_pipeline_.get());
    thread.setViewport();

    auto vp = camera_.getViewProj();

    for (auto& chunk_row : chunks_) {
        for (auto& chunk : chunk_row) {
            for (auto& object : chunk.objects) {
                if (object.model->state == UploadState::OK) {
                    for (auto& mesh : object.model->meshes) {
                        if (mesh.index_count == 0) {
                            continue;
                        }

                        struct {
                            mat4f mvp;
                            f32 metalness;
                            f32 roughness;
                            f32 emissive;
                        } pc{};

                        pc.mvp = vp * glm::translate(object.transform, {chunk.pos.x, 1, chunk.pos.y});
                        pc.metalness = mesh.metalness;
                        pc.roughness = mesh.roughness;
                        pc.emissive = mesh.emissive;

                        thread.pushConstants(&pc);

                        thread.bindVertexBuffer(mesh.vertex_buffer.get());
                        thread.bindIndexBuffer(mesh.index_buffer.get());

                        thread.drawIndexed(1, mesh.index_count, 0);
                    }
                }
            }
        }
    }

    thread.endRendering();

    g_.albedo->transitionState(thread.cmd(), Image::State::SHADER_READ);
    g_.normal->transitionState(thread.cmd(), Image::State::SHADER_READ);
    g_.material->transitionState(thread.cmd(), Image::State::SHADER_READ);
    target_image_->transitionState(thread.cmd(), Image::State::COLOR_ATT);

    {
        std::vector<AttachmentInfo> attachments;
        auto& c0 = attachments.emplace_back();
        c0.image = target_image_.get();
        c0.load = vk::AttachmentLoadOp::eClear;
        c0.store = vk::AttachmentStoreOp::eStore;
        thread.setColorAttachments(std::move(attachments));
    }

    thread.beginRendering();

    thread.setViewport();

    thread.setPipeline(lighting_pipeline_.get());

    thread.pushDescriptor(0, lighting_wds_);

    thread.draw(1, 3);

    thread.submit();
}

void SceneRenderer::setChunk(int x, int y, Chunk&& chunk) {
    if (x < 0 || x >= static_cast<int>(chunks_.size()) || y < 0 || y >= static_cast<int>(chunks_[0].size())) {
        MLE_E("Chunk coordinates out of bounds: ({}, {})", x, y);
        return;
    }
    chunks_.at(x).at(y) = std::move(chunk);
}
}  // namespace mle::renderer
