#include "SceneRenderer.h"

#include <cstddef>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "mle/common/Logger.h"
#include "mle/common/math/Types.h"
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
    cube_map_.shutdown();
}

void SceneRenderer::init(const CI& ci) {
    MLE_D("Initializing scene renderer with image_extent: {}, chunk_count: {}, chunk_size: {}", ci.image_extent, ci.chunk_count, ci.chunk_size);

    camera_.setAspect(static_cast<f32>(ci.image_extent.x) / static_cast<f32>(ci.image_extent.y));
    camera_.setFov(60.0F);
    camera_.setNear(0.1F);
    camera_.setFar(1000.0F);

    MLE_D("Creating images");
    Image::CI image_ci{};
    image_ci.extent = ci.image_extent;
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
    createPlanePipeline();
    createLightingPipeline();
    createWDS();

    createCubeMap(ci.cube_map);

    chunk_size_ = as<f32>(ci.chunk_size);

    chunks_.resize(ci.chunk_count.x);
    for (auto& chunk_row : chunks_) {
        chunk_row.resize(ci.chunk_count.y);
    }

    MLE_D("Scene renderer initialized");
}

void SceneRenderer::createCubeMap(const std::string& name) {
    if (name.empty()) {
        return;
    }

    MLE_D("Creating cube map");

    cube_map_.init(name);
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

void SceneRenderer::createPlanePipeline() {
    MLE_D("Creating plane pipeline");

    Pipeline::CI pipeline_ci;
    pipeline_ci.vertex_shader = getShader("mle/scene/plane.vert");
    pipeline_ci.fragment_shader = getShader("mle/scene/plane.frag");
    pipeline_ci.depth = true;
    pipeline_ci.color_attachment_formats.emplace_back(vk::Format::eR8G8B8A8Unorm);
    pipeline_ci.color_attachment_formats.emplace_back(vk::Format::eR8G8B8A8Unorm);
    pipeline_ci.color_attachment_formats.emplace_back(vk::Format::eR8G8B8A8Unorm);
    pipeline_ci.blend_attachments = makeDefaultBlendAttachmentStates(3, false);
    pipeline_ci.topology = vk::PrimitiveTopology::eTriangleStrip;

    plane_pipeline_ = Pipeline::createHnd(pipeline_ci);
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
    thread.setViewport();

    auto vp = camera_.getViewProj();
    auto frustum = camera_.getFrustum();

    std::vector<Object> rendered_objects;
    std::vector<Light> rendered_lights;
    std::vector<RenderReadyChunk::Plane> rendered_planes;

    // TODO: MT
    for (usize i = 0; i < chunks_.size(); i++) {
        for (usize j = 0; j < chunks_[i].size(); j++) {
            auto& chunk = chunks_[i][j];

            if (!chunk.ready) {
                continue;
            }

            auto result = renderChunk({as<i32>(i), as<i32>(j)}, chunk, vp, frustum);
            if (result) {
                rendered_objects.insert(rendered_objects.end(), result->objects.begin(), result->objects.end());
                rendered_lights.insert(rendered_lights.end(), result->lights.begin(), result->lights.end());
                rendered_planes.push_back(result->plane);
            } else {
                // MLE_C("Chunk at ({}, {}) is not in frustum", i, j);
            }
        }
    }

    if (!rendered_objects.empty()) {
        MLE_T("Rendering {} objects", rendered_objects.size());
        thread.setPipeline(g_pipeline_.get());

        for (auto& obj : rendered_objects) {
            auto& model = obj.model;

            struct {
                mat4f mvp;
                f32 metalness;
                f32 roughness;
                f32 emissive;
            } pc{};

            pc.mvp = obj.transform;

            for (auto& mesh : obj.model->meshes) {
                if (model->state != UploadState::OK) {
                    continue;
                }

                pc.metalness = mesh.metalness;
                pc.roughness = mesh.roughness;
                pc.emissive = mesh.emissive;

                thread.pushConstants(&pc);

                thread.bindVertexBuffer(mesh.vertex_buffer.get());
                thread.bindIndexBuffer(mesh.index_buffer.get());

                thread.drawIndexed(1, mesh.index_count, 0);
            }
        }

        // This could be intanced but this is probably temporary and fine for now
    }

    if (!rendered_planes.empty()) {
        MLE_T("Rendering {} planes", rendered_planes.size());

        thread.setPipeline(plane_pipeline_.get());

        struct PlanePushConstants {
            mat4f vp;
            vec2f left_bottom;
            vec2f plane_size;
            vec3f color1;
            f32 _pad1;
            vec3f color2;
            int divisions;
        } plane_pc{};
        plane_pc.vp = vp;
        plane_pc.plane_size = {chunk_size_, chunk_size_};
        plane_pc.divisions = as<int>(chunk_size_) * 10;

        for (auto& plane : rendered_planes) {
            plane_pc.left_bottom = plane.left_bottom;
            plane_pc.color1 = plane.colors[0];
            plane_pc.color2 = plane.colors[1];

            thread.pushConstants(&plane_pc);

            thread.draw(1, 4);
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
        thread.setDepthAttachment({});
    }

    thread.beginRendering();

    thread.setViewport();

    thread.setPipeline(lighting_pipeline_.get());

    thread.pushDescriptor(0, lighting_wds_);

    thread.draw(1, 3);

    thread.endRendering();

    renderCubeMap(thread);

    thread.submit();
}

void SceneRenderer::renderCubeMap(RenderingThread& thread) {
    if (!cube_map_.ready()) {
        return;
    }

    std::vector<AttachmentInfo> attachments;
    auto& c0 = attachments.emplace_back();
    c0.image = target_image_.get();
    c0.load = vk::AttachmentLoadOp::eLoad;
    c0.store = vk::AttachmentStoreOp::eStore;
    thread.setColorAttachments(std::move(attachments));

    AttachmentInfo depth_attachment{};
    depth_attachment.image = g_.depth.get();
    depth_attachment.load = vk::AttachmentLoadOp::eLoad;
    depth_attachment.store = vk::AttachmentStoreOp::eStore;
    thread.setDepthAttachment(depth_attachment);

    thread.beginRendering({});

    thread.setViewport();

    thread.setPipeline(CubeMap::getPipeline());

    vk::DescriptorImageInfo ii;
    ii.setImageView(cube_map_.getView());
    ii.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::WriteDescriptorSet wds;
    wds.setImageInfo(ii);
    wds.setDstBinding(0);
    wds.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    wds.setDstArrayElement(0);
    wds.setDescriptorCount(1);

    thread.pushDescriptor(0, {wds});

    thread.bindIndexBuffer(CubeMap::getIndexBuffer());

    struct PushConstants {
        mat4f view;
        mat4f proj;
    } pc{.view = camera_.getView(), .proj = camera_.getProj()};

    thread.pushConstants(&pc);

    thread.drawIndexed(1, CubeMap::getIndexCount(), 0);
}

std::optional<SceneRenderer::RenderReadyChunk> SceneRenderer::renderChunk(vec2i position, Chunk& chunk, const mat4f& vp, const Frustum& frustum) const {
    MLE_D("Rendering chunk {}");

    static f32 rotation = 0.0F;
    // rotation += 0.001F;

    vec2f chunk_global_pos{as<f32>(position.x) * chunk_size_, as<f32>(position.y) * chunk_size_};

    auto chunk_plane = RectPlane::fromSquareLBCorner({chunk_global_pos.x, 0, chunk_global_pos.y}, {0.0F, 1.0F, 0.0F}, chunk_size_);

    auto plane_in_frustum = frustum.contains(chunk_plane);
    if (!plane_in_frustum) {
        return std::nullopt;
    }

    RenderReadyChunk ret{};
    ret.objects.reserve(chunk.objects.size());

    for (const auto& object : chunk.objects) {
        if (object.second.model->state == UploadState::OK) {
            // TODO: frustum the obj
            // animate

            auto transform = glm::rotate(glm::mat4(1.0F), rotation, {0.0F, 1.0F, 0.0F});
            transform = glm::translate(object.second.transform, {chunk_global_pos.x, 0.0F, chunk_global_pos.y}) * transform;

            auto& obj = ret.objects.emplace_back(object.second);
            obj.transform = vp * transform;
        }
    }

    ret.objects.shrink_to_fit();

    ret.plane.left_bottom = chunk_global_pos;
    ret.plane.colors = chunk.colors;

    return ret;
}

}  // namespace mle::renderer
