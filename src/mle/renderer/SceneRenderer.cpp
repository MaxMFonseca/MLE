#include "SceneRenderer.h"

#include <algorithm>
#include <cstddef>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "mle/common/Logger.h"
#include "mle/common/math/Types.h"
#include "mle/common/math/Types3D.h"
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

    detail::getDevice().destroy(lighting_.dsl);
    g_pipeline_.reset();
    plane_pipeline_.reset();
    lighting_.pipeline.reset();
    target_image_.reset();
    g_.albedo.reset();
    g_.normal.reset();
    g_.material.reset();
    g_.depth.reset();
    cube_map_.shutdown();
    debug_.polygon_pipeline.reset();
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
    image_ci.format = getDefaultColorFormat();
    g_.albedo = Image::createHnd(image_ci);
    g_.normal = Image::createHnd(image_ci);
    g_.material = Image::createHnd(image_ci);
    image_ci.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
    image_ci.format = getDepthFormat();
    g_.depth = Image::createHnd(image_ci);

    image_ci.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
    image_ci.format = getDefaultColorFormat();
    target_image_ = Image::createHnd(image_ci);

    MLE_D("Creating Pipelines");
    createSun();
    createGPipeline();
    createPlanePipeline();
    createLightingPipeline();
    createDebug();

    createCubeMap(ci.cube_map);

    chunk_size_ = as<f32>(ci.chunk_size);

    chunks_.resize(ci.chunk_count.x);
    for (auto& chunk_row : chunks_) {
        chunk_row.resize(ci.chunk_count.y);
    }

    MLE_D("Scene renderer initialized");
}

void SceneRenderer::createSun() {
    MLE_D("Creating sun data");

    Image::CI image_ci{};
    image_ci.extent = {2048, 2048};
    image_ci.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
    image_ci.format = getDepthFormat();
    sun_.image = Image::createHnd(image_ci);

    sun_.color = Color::WHITE;

    Pipeline::CI pipeline_ci;
    pipeline_ci.vertex_shader = getShader("mle/scene/sun.vert");
    pipeline_ci.depth = true;

    sun_.pipeline = Pipeline::createHnd(pipeline_ci);
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
    pipeline_ci.color_attachment_formats.emplace_back(g_.albedo->getFormat());
    pipeline_ci.color_attachment_formats.emplace_back(g_.normal->getFormat());
    pipeline_ci.color_attachment_formats.emplace_back(g_.material->getFormat());
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
    pipeline_ci.color_attachment_formats.emplace_back(g_.albedo->getFormat());
    pipeline_ci.color_attachment_formats.emplace_back(g_.normal->getFormat());
    pipeline_ci.color_attachment_formats.emplace_back(g_.material->getFormat());
    pipeline_ci.blend_attachments = makeDefaultBlendAttachmentStates(3, false);
    pipeline_ci.topology = vk::PrimitiveTopology::eTriangleStrip;

    plane_pipeline_ = Pipeline::createHnd(pipeline_ci);
}

void SceneRenderer::createLightingPipeline() {
    MLE_D("Creating lighting pipeline");

    vk::SamplerCreateInfo sampler_ci;
    sampler_ci.magFilter = vk::Filter::eLinear;
    sampler_ci.minFilter = vk::Filter::eLinear;
    sampler_ci.mipmapMode = vk::SamplerMipmapMode::eLinear;
    sampler_ci.addressModeU = vk::SamplerAddressMode::eRepeat;
    sampler_ci.addressModeV = vk::SamplerAddressMode::eRepeat;
    sampler_ci.addressModeW = vk::SamplerAddressMode::eRepeat;
    sampler_ci.maxLod = VK_LOD_CLAMP_NONE;

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(6);

    auto linear_sampler = getLinearSampler();
    // auto nearest_sampler = getNearestSampler();
    auto shadow_sampler = getShadowSampler();

    // layout(set = 0, binding = 0) uniform sampler2D in_albedo;
    auto& s00 = bindings.emplace_back();
    s00.binding = 0;
    s00.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    s00.descriptorCount = 1;
    s00.stageFlags = vk::ShaderStageFlagBits::eFragment;
    s00.setImmutableSamplers(linear_sampler);

    // layout(set = 0, binding = 1) uniform sampler2D in_normal;
    auto& s01 = bindings.emplace_back();
    s01.binding = 1;
    s01.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    s01.descriptorCount = 1;
    s01.stageFlags = vk::ShaderStageFlagBits::eFragment;
    s01.setImmutableSamplers(linear_sampler);

    // layout(set = 0, binding = 2) uniform sampler2D in_material;
    auto& s02 = bindings.emplace_back();
    s02.binding = 2;
    s02.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    s02.descriptorCount = 1;
    s02.stageFlags = vk::ShaderStageFlagBits::eFragment;
    s02.setImmutableSamplers(linear_sampler);

    // layout(set = 0, binding = 3) uniform sampler2D in_depth;
    auto& s03 = bindings.emplace_back();
    s03.binding = 3;
    s03.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    s03.descriptorCount = 1;
    s03.stageFlags = vk::ShaderStageFlagBits::eFragment;
    s03.setImmutableSamplers(linear_sampler);

    // layout(set = 0, binding = 4) uniform GlobalUniforms globals;
    auto& s04 = bindings.emplace_back();
    s04.binding = 4;
    s04.descriptorType = vk::DescriptorType::eUniformBuffer;
    s04.descriptorCount = 1;
    s04.stageFlags = vk::ShaderStageFlagBits::eFragment;

    // layout(set = 0, binding = 5) uniform sampler2DShadow sun_shadow_map;
    auto& s05 = bindings.emplace_back();
    s05.binding = 5;
    s05.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    s05.descriptorCount = 1;
    s05.stageFlags = vk::ShaderStageFlagBits::eFragment;
    s05.setImmutableSamplers(shadow_sampler);

    vk::DescriptorSetLayoutCreateInfo dsl_ci;
    dsl_ci.setBindings(bindings);
    dsl_ci.setFlags(vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR);

    lighting_.dsl = unwrap(detail::getDevice().createDescriptorSetLayout(dsl_ci));

    Pipeline::CI pipeline_ci;
    pipeline_ci.vertex_shader = getShader("mle/fs_triangle.vert");
    pipeline_ci.fragment_shader = getShader("mle/scene/light.frag");
    pipeline_ci.depth = true;
    pipeline_ci.color_attachment_formats.emplace_back(target_image_->getFormat());
    pipeline_ci.blend_attachments = makeDefaultBlendAttachmentStates(1);
    pipeline_ci.topology = vk::PrimitiveTopology::eTriangleList;
    pipeline_ci.descriptor_set_layouts.emplace_back(lighting_.dsl);
    pipeline_ci.depth_bias = true;

    lighting_.pipeline = Pipeline::createHnd(pipeline_ci);
}

void SceneRenderer::render() {
    RenderingThread thread;
    thread.init();

    auto vp = camera_.getViewProj();
    auto frustum = camera_.getFrustum();

    renderSun(thread, frustum);

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

    renderLighting(thread);

    renderCubeMap(thread);

    // renderDebug(thread, vp);

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

void SceneRenderer::createDebug() {
    MLE_D("Creating debug renderer");

    Pipeline::CI pipeline_ci;
    pipeline_ci.vertex_shader = getShader("mle/scene/debug/polygon.vert");
    pipeline_ci.fragment_shader = getShader("mle/scene/debug/polygon.frag");
    pipeline_ci.color_attachment_formats.emplace_back(getDefaultColorFormat());
    pipeline_ci.blend_attachments = makeDefaultBlendAttachmentStates(1);
    pipeline_ci.topology = vk::PrimitiveTopology::eTriangleFan;
    pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;

    debug_.polygon_pipeline = Pipeline::createHnd(pipeline_ci);
}

void SceneRenderer::renderDebug(RenderingThread& thread, const mat4f& vp) {
    if (!debug_.polygons.empty()) {
        std::vector<AttachmentInfo> attachments;
        auto& c0 = attachments.emplace_back();
        c0.image = target_image_.get();
        c0.load = vk::AttachmentLoadOp::eLoad;
        c0.store = vk::AttachmentStoreOp::eStore;
        thread.setColorAttachments(std::move(attachments));
        thread.setDepthAttachment({});

        thread.beginRendering();

        thread.setViewport();

        thread.setPipeline(debug_.polygon_pipeline.get());

        for (const auto& pol : debug_.polygons) {
            struct PushConstants {
                mat4f vp;
                vec4f color;
            } pc{};

            pc.vp = vp;
            pc.color = pol.second;

            auto vertices = pol.first.vertices();

            Buffer::CI vertex_buffer_ci = {};
            vertex_buffer_ci.size = sizeof(vec3f) * vertices.size();
            vertex_buffer_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer;
            vertex_buffer_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY_HOST_WRITE_SEQ;
            auto buffer = Buffer::createHnd(vertex_buffer_ci);

            buffer->update(vertices.data());

            thread.pushConstants(&pc);
            thread.bindVertexBuffer(buffer.get());
            thread.draw(1, as<int>(vertices.size()));

            deleteAfterFrame(std::move(buffer));
        }
        thread.endRendering();

        debug_.polygons.clear();
    }
}

void SceneRenderer::renderLighting(RenderingThread& thread) {
    // TODO: FIND A WAY TO MAKE THIS AUTO

    std::vector<vk::WriteDescriptorSet> writes;

    // layout(set = 0, binding = 0) uniform sampler2D in_albedo;
    g_.albedo->transitionState(thread.cmd(), Image::State::SHADER_READ);
    vk::DescriptorImageInfo albedo_info;
    albedo_info.setImageView(g_.albedo->getView());
    albedo_info.setImageLayout(g_.albedo->getCurrentLayout());
    auto& b0 = writes.emplace_back();
    b0.dstBinding = 0;
    b0.dstArrayElement = 0;
    b0.descriptorCount = 1;
    b0.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b0.pImageInfo = &albedo_info;

    // layout(set = 0, binding = 1) uniform sampler2D in_normal;
    g_.normal->transitionState(thread.cmd(), Image::State::SHADER_READ);
    vk::DescriptorImageInfo normal_info;
    normal_info.setImageView(g_.normal->getView());
    normal_info.setImageLayout(g_.normal->getCurrentLayout());
    auto& b1 = writes.emplace_back();
    b1.dstBinding = 1;
    b1.dstArrayElement = 0;
    b1.descriptorCount = 1;
    b1.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b1.pImageInfo = &normal_info;

    // layout(set = 0, binding = 2) uniform sampler2D in_material;
    g_.material->transitionState(thread.cmd(), Image::State::SHADER_READ);
    vk::DescriptorImageInfo material_info;
    material_info.setImageView(g_.material->getView());
    material_info.setImageLayout(g_.material->getCurrentLayout());
    auto& b2 = writes.emplace_back();
    b2.dstBinding = 2;
    b2.dstArrayElement = 0;
    b2.descriptorCount = 1;
    b2.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b2.pImageInfo = &material_info;

    // layout(set = 0, binding = 3) uniform sampler2D in_depth;
    g_.depth->transitionState(thread.cmd(), Image::State::SHADER_READ);
    vk::DescriptorImageInfo depth_info;
    depth_info.setImageView(g_.depth->getView());
    depth_info.setImageLayout(g_.depth->getCurrentLayout());
    auto& b3 = writes.emplace_back();
    b3.dstBinding = 3;
    b3.dstArrayElement = 0;
    b3.descriptorCount = 1;
    b3.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b3.pImageInfo = &depth_info;

    // layout(set = 0, binding = 4) uniform GlobalUniforms globals;
    LightingGlobalUniforms globals{};
    globals.view = camera_.getView();
    globals.proj = camera_.getProj();
    globals.view_proj = camera_.getViewProj();
    globals.inv_view_proj = glm::inverse(globals.view_proj);
    globals.sun_light_matrix = sun_.matrix;
    globals.sun_direction = sun_.direction;
    globals.sun_color = sun_.color;
    auto buffer_slice = getHostVisibleBuffer(sizeof(LightingGlobalUniforms), vk::BufferUsageFlagBits::eUniformBuffer);
    buffer_slice.buffer->update(&globals, buffer_slice.size, buffer_slice.offset);
    vk::DescriptorBufferInfo globals_info;
    globals_info.setBuffer(buffer_slice.buffer->getVkHnd());
    globals_info.setOffset(buffer_slice.offset);
    globals_info.setRange(buffer_slice.size);
    auto& b4 = writes.emplace_back();
    b4.dstBinding = 4;
    b4.dstArrayElement = 0;
    b4.descriptorCount = 1;
    b4.descriptorType = vk::DescriptorType::eUniformBuffer;
    b4.pBufferInfo = &globals_info;

    // layout(set = 0, binding = 5) uniform sampler2DShadow sun_shadow_map;
    sun_.image->transitionState(thread.cmd(), Image::State::SHADER_READ);
    vk::DescriptorImageInfo sun_shadow_info;
    sun_shadow_info.setImageView(sun_.image->getView());
    sun_shadow_info.setImageLayout(sun_.image->getCurrentLayout());
    auto& b5 = writes.emplace_back();
    b5.dstBinding = 5;
    b5.dstArrayElement = 0;
    b5.descriptorCount = 1;
    b5.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b5.pImageInfo = &sun_shadow_info;

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

    thread.setPipeline(lighting_.pipeline.get());

    thread.pushDescriptor(0, writes);

    thread.draw(1, 3);

    thread.endRendering();
}

void SceneRenderer::renderSun(RenderingThread& thread, const Frustum& camera_frustum) {
    auto camera_frustum_inter_y0 = camera_frustum.intersectWithY0();
    auto camera_frustum_inter_y0_xz = camera_frustum_inter_y0.xz();

    // Debug
    if (IS_DEBUG_BUILD) {
        if (camera_frustum_inter_y0.vertCount() != 0) {
            debug_.polygons.emplace_back(camera_frustum_inter_y0, Color::RED.withA(0.3));

            for (auto p : camera_frustum_inter_y0.vertices()) {
                MLE_C("Frustum intersection point: {}", p);
            }
        }
    }

    Camera sun_camera;
    sun_camera.setProjType(Camera::ProjType::ORTH);
    sun_camera.setTarget(camera_frustum_inter_y0.center());
    sun_camera.setPosition(camera_frustum_inter_y0.center() - sun_.direction * 50.0F);
    sun_camera.setUp({0.0F, -1.0F, 0.0F});
    sun_camera.setNear(0.1);
    sun_camera.setFar(200.0F);
    auto sun_size = 50.0F;
    sun_camera.setRect(sun_size);

    Polygon3D sun_frustum_inter_y0;
    auto sun_frustum_inter_y0_xz = sun_frustum_inter_y0.xz();

    int max_iterations = 100;
    while (!isInside(sun_frustum_inter_y0_xz, camera_frustum_inter_y0_xz) && max_iterations-- > 0) {
        sun_size *= 1.1F;
        sun_camera.setRect(sun_size);
        sun_frustum_inter_y0 = sun_camera.getFrustum().intersectWithY0();
        sun_frustum_inter_y0_xz = sun_frustum_inter_y0.xz();
    }
    MLE_T("Final: {}", sun_size);
    MLE_C("Sun position: {}", sun_camera.getPos());

    mat4f sun_vp = sun_camera.getViewProj();
    sun_.matrix = sun_vp;

    ///

    std::vector<Object> objects;
    std::vector<std::array<vec2f, 4>> planes;

    // cull all chunks that the sun cant see
    for (usize i = 0; i < chunks_.size(); ++i) {
        for (usize j = 0; j < chunks_[i].size(); ++j) {
            auto& chunk = chunks_[i][j];
            if (!chunk.ready) {
                continue;
            }

            vec2f world_pos00{as<f32>(i) * chunk_size_, as<f32>(j) * chunk_size_};
            vec2f world_pos10{world_pos00.x + chunk_size_, world_pos00.y};
            vec2f world_pos11{world_pos00.x + chunk_size_, world_pos00.y + chunk_size_};
            vec2f world_pos01{world_pos00.x, world_pos00.y + chunk_size_};
            auto world_pos = std::array{world_pos00, world_pos10, world_pos11, world_pos01};

            if (std::ranges::any_of(world_pos,
                                    [&sun_frustum_inter_y0_xz](const vec2f& world_pos) { return isPointInsidePolygon(world_pos, sun_frustum_inter_y0_xz); })) {
                planes.emplace_back(world_pos);
                for (const auto& object : chunk.objects) {
                    if (object.second.model->state == UploadState::OK) {
                        auto transform = glm::translate(object.second.transform, {world_pos00.x, 0.0F, world_pos00.y});

                        auto& obj = objects.emplace_back(object.second);
                        obj.transform = sun_vp * transform;
                    }
                }
            }
        }
    }

    std::array plane_indices = {0, 1, 2, 0, 2, 3};
    std::array<PCNVertex, 4> plane_vertices;
    plane_vertices[0] = {.pos = vec3f{0.0F, 0.0F, 0.0F}, .color = Color::WHITE, .normal = vec3f{0.0F, 1.0F, 0.0F}};
    plane_vertices[1] = {.pos = vec3f{0.0F, 0.0F, chunk_size_}, .color = Color::WHITE, .normal = vec3f{0.0F, 1.0F, 0.0F}};
    plane_vertices[2] = {.pos = vec3f{chunk_size_, 0.0F, chunk_size_}, .color = Color::WHITE, .normal = vec3f{0.0F, 1.0F, 0.0F}};
    plane_vertices[3] = {.pos = vec3f{chunk_size_, 0.0F, 0.0F}, .color = Color::WHITE, .normal = vec3f{0.0F, 1.0F, 0.0F}};

    BufferSlice plane_index_buffer = getHostVisibleBuffer(sizeof(plane_indices), vk::BufferUsageFlagBits::eIndexBuffer);
    plane_index_buffer.buffer->update(plane_indices.data(), plane_index_buffer.size, plane_index_buffer.offset);
    BufferSlice plane_vertex_buffer = getHostVisibleBuffer(sizeof(PCNVertex) * 4, vk::BufferUsageFlagBits::eVertexBuffer);
    plane_vertex_buffer.buffer->update(plane_vertices.data(), plane_vertex_buffer.size, plane_vertex_buffer.offset);

    ///

    AttachmentInfo sun_depth_attachment{};
    sun_depth_attachment.image = sun_.image.get();
    sun_depth_attachment.load = vk::AttachmentLoadOp::eClear;
    sun_depth_attachment.store = vk::AttachmentStoreOp::eStore;
    sun_depth_attachment.clear_value.setDepthStencil({1.0F, 0});
    sun_.image->transitionState(thread.cmd(), Image::State::DEPTH_ATT);
    thread.setDepthAttachment(sun_depth_attachment);
    thread.setColorAttachments({});

    thread.beginRendering({});

    thread.setViewport();
    thread.setPipeline(sun_.pipeline.get());

    for (const auto& obj : objects) {
        struct {
            mat4f mvp;
        } pc{};

        pc.mvp = obj.transform;

        thread.pushConstants(&pc);

        for (const auto& mesh : obj.model->meshes) {
            thread.bindVertexBuffer(mesh.vertex_buffer.get());
            thread.bindIndexBuffer(mesh.index_buffer.get());

            thread.drawIndexed(1, mesh.index_count);
        }
    }

    thread.bindVertexBuffer(plane_vertex_buffer.buffer, plane_vertex_buffer.offset);
    thread.bindIndexBuffer(plane_index_buffer.buffer, plane_index_buffer.offset);

    for (const auto& plane : planes) {
        struct {
            mat4f mvp;
        } pc{};

        pc.mvp = sun_vp;
        pc.mvp = glm::translate(pc.mvp, {plane[0].x, 0.0F, plane[0].y});

        thread.pushConstants(&pc);
        thread.drawIndexed(1, 6, 0);
    }

    thread.endRendering();
}
}  // namespace mle::renderer
