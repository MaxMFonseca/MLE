#include "SceneRenderer.h"

#include <algorithm>
#include <cstddef>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "mle/common/Logger.h"
#include "mle/common/math/Types.h"
#include "mle/common/math/Types2D.h"
#include "mle/common/math/Types3D.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"
#include "spdlog/details/os.h"

namespace mle::renderer {
void SceneRenderer::shutdown() {
    MLE_D("Shutting down scene renderer");

    detail::waitIdle();

    pipelines_ = {};
}

void SceneRenderer::init(const CI& ci) {
    camera_.setProjType(Camera::ProjType::PERSPECTIVE);
    camera_.setAspect(static_cast<f32>(ci.image_extent.x) / static_cast<f32>(ci.image_extent.y));
    camera_.setFov(60.0F);
    camera_.setNear(1.F);
    camera_.setFar(200.0F);
    camera_.setEye({0.0F, 5.0F, -5.0F});
    camera_.setTarget({0.0F, 0.0F, 0.0F});
    camera_.setUp({0, 1, 0});

    initSun();
    initG(ci);
    initPlane();
    initSun();
    initLighting(ci);
    initSkybox(ci);
    // initDebug();

    MLE_D("Scene renderer initialized");
}

void SceneRenderer::initSun() {
    MLE_D("Creating sun data");

    Image::CI image_ci{};
    image_ci.extent = {4096, 4096};
    image_ci.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
    image_ci.format = getDepthFormat();
    shadow_img_ = Image::createHnd(image_ci);

    pipelines_.sun = getPipeline("mle/scene/sun");
}

void SceneRenderer::initSkybox(const CI& ci) {
    if (ci.skybox.empty()) {
        return;
    }

    MLE_D("Creating cube map");

    pipelines_.skybox = getPipeline("mle/scene/cube_map");
    skybox_.init(ci.skybox);
}

void SceneRenderer::initG(const CI& ci) {
    MLE_D("Creating G-Buffer pipeline");

    pipelines_.vox = getPipeline("mle/scene/vox");

    Image::CI image_ci{};
    image_ci.extent = ci.image_extent;
    image_ci.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
    image_ci.format = getDefaultColorFormat();
    albedo_img_ = Image::createHnd(image_ci);
    normal_img_ = Image::createHnd(image_ci);
    material_img_ = Image::createHnd(image_ci);
    image_ci.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
    image_ci.format = getDepthFormat();
    depth_img_ = Image::createHnd(image_ci);
}

void SceneRenderer::initPlane() {
    MLE_D("Creating plane pipeline");

    pipelines_.plane = getPipeline("mle/scene/plane");
}

void SceneRenderer::initLighting(const CI& ci) {
    MLE_D("Creating lighting pipeline");

    Image::CI image_ci{};
    image_ci.extent = ci.image_extent;
    image_ci.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
    image_ci.format = getDefaultColorFormat();
    target_img_ = Image::createHnd(image_ci);

    pipelines_.lighting = getPipeline("mle/scene/lighting");
}

void SceneRenderer::render() {
    RenderingData rdata{dimensions_.at(0)};
    rdata.dim_id = 0U;

    rdata.thread.init();

    rdata.view = camera_.getView();
    rdata.proj = camera_.getProj();
    rdata.view_proj = camera_.getViewProj();
    rdata.inv_view_proj = camera_.getInvViewProj();
    rdata.camera_frustum = camera_.getFrustum();

    auto camera_frustum_inter_y0 = rdata.camera_frustum.intersectWithY0();
    rdata.camera_center_y0 = camera_frustum_inter_y0.center();
    rdata.camera_frustum_inter_y0 = camera_frustum_inter_y0.xz();

    renderSun(rdata);
    renderGBuffer(rdata);
    renderLighting(rdata);
    renderSkybox(rdata);
    // renderDebug(rdata);

    rdata.thread.submit();
}

void SceneRenderer::renderSun(RenderingData& rdata) {  // NOLINT
    calcSunCamera(rdata);

    auto sun_frustum_inter_y0 = rdata.sun_frustum.intersectWithY0();
    Polygon2f sun_frustum_polygon(sun_frustum_inter_y0.xz());
    AABB2D sun_frustum_aabb = sun_frustum_polygon.boundingBox();

    f32 view_size_world = CHUNK_SIZE * WORLD_VIEW_SIZE;

    int sun_aabb_inter_view_range_x0 = as<int>(std::floor(sun_frustum_aabb.min().x / view_size_world));
    int sun_aabb_inter_view_range_x1 = as<int>(std::ceil(sun_frustum_aabb.max().x / view_size_world));
    int sun_aabb_inter_view_range_z0 = as<int>(std::floor(sun_frustum_aabb.min().y / view_size_world));
    int sun_aabb_inter_view_range_z1 = as<int>(std::ceil(sun_frustum_aabb.max().y / view_size_world));

    std::vector<usize> sun_views;

    for (int x_idx = sun_aabb_inter_view_range_x0; x_idx <= sun_aabb_inter_view_range_x1; ++x_idx) {
        for (int z_idx = sun_aabb_inter_view_range_z0; z_idx <= sun_aabb_inter_view_range_z1; ++z_idx) {
            int x1 = x_idx * as<int>(WORLD_VIEW_SIZE);
            int x2 = x1 + as<int>(WORLD_VIEW_SIZE);
            int z1 = z_idx * as<int>(WORLD_VIEW_SIZE);
            int z2 = z1 + as<int>(WORLD_VIEW_SIZE);
            x1 *= CHUNK_SIZE;
            x2 *= CHUNK_SIZE;
            z1 *= CHUNK_SIZE;
            z2 *= CHUNK_SIZE;

            AABB2D view_aabb{std::array{vec2f{x1, z1}, vec2f{x2, z2}}};

            if (view_aabb.intersects(sun_frustum_aabb)) {
                const auto* it = std::ranges::find_if(
                    rdata.dim.world_views, [&](const WorldView& view) { return view.valid && view.position.x == x_idx && view.position.y == z_idx; });
                if (it == rdata.dim.world_views.end()) {
                    // TODO: request the view
                    continue;
                }

                sun_views.push_back(std::distance(rdata.dim.world_views.begin(), it));
            }
        }
    }

    usize obj_count = 0;

    for (auto view_idx : sun_views) {
        const auto& view = rdata.dim.world_views.at(view_idx);
        if (!view.valid) {
            continue;
        }

        vec2i view_base_pos = {view.position.x * WORLD_VIEW_SIZE, view.position.y * WORLD_VIEW_SIZE};

        for (usize c_x = 0; c_x < WORLD_VIEW_SIZE; c_x++) {
            for (usize c_z = 0; c_z < WORLD_VIEW_SIZE; c_z++) {
                const auto& chunk = view.chunks.at(c_x).at(c_z);

                if (chunk.ready == false) {
                    continue;
                }

                vec2i chunk_pos = {view_base_pos.x + c_x, view_base_pos.y + c_z};

                int chunk_x1 = chunk_pos.x * as<int>(CHUNK_SIZE);
                int chunk_x2 = chunk_x1 + as<int>(CHUNK_SIZE);
                int chunk_z1 = chunk_pos.y * as<int>(CHUNK_SIZE);
                int chunk_z2 = chunk_z1 + as<int>(CHUNK_SIZE);

                AABB2D chunk_aabb{std::array{vec2f{chunk_x1, chunk_z1}, vec2f{chunk_x2, chunk_z2}}};

                if (chunk_aabb.intersects(sun_frustum_aabb)) {
                    rdata.rendered_planes.emplace_back(chunk_pos);

                    vec3f chunk_world_pos{as<f32>(chunk_pos.x) * CHUNK_SIZE, 0.0F, as<f32>(chunk_pos.y) * CHUNK_SIZE};

                    for (const auto& object : chunk.objects) {
                        if (object.second.model->state == UploadState::OK) {
                            mat4f obj_transform = glm::translate(object.second.transform, chunk_world_pos);

                            auto aabb_xz = object.second.model->aabb.translateToXZ(obj_transform);

                            if (!aabb_xz.intersects(sun_frustum_aabb)) {
                                continue;
                            }

                            rdata.rendered_objs[object.second.model].emplace_back(obj_transform);
                            obj_count++;
                        }
                    }
                }
            }
        }
    }

    rdata.rendered_transforms = getHostVisibleBuffer(obj_count * sizeof(mat4f), vk::BufferUsageFlagBits::eVertexBuffer);
    auto& transforms_buffer = rdata.rendered_transforms;

    {
        usize curr_offset = transforms_buffer.offset;
        for (const auto& model_type : rdata.rendered_objs) {
            if (model_type.second.empty()) {
                continue;
            }

            usize buffer_size = model_type.second.size() * sizeof(mat4f);

            transforms_buffer.buffer->update(model_type.second.data(), buffer_size, curr_offset);
            curr_offset += buffer_size;
        }
    }

    AttachmentInfo sun_depth_attachment{};
    sun_depth_attachment.image = shadow_img_.get();
    sun_depth_attachment.load = vk::AttachmentLoadOp::eClear;
    sun_depth_attachment.store = vk::AttachmentStoreOp::eStore;
    sun_depth_attachment.clear_value.setDepthStencil({1.0F, 0});
    shadow_img_->transitionState(rdata.thread.cmd(), Image::State::DEPTH_ATT);
    rdata.thread.setDepthAttachment(sun_depth_attachment);
    rdata.thread.setColorAttachments({});

    rdata.thread.beginRendering({});

    rdata.thread.setViewport();

    if (!rdata.rendered_objs.empty()) {
        rdata.thread.setPipeline(pipelines_.sun);
        struct {
            mat4f vp;
        } pc{};

        pc.vp = rdata.sun_vp;

        rdata.thread.pushConstants(&pc);

        usize transforms_offset = transforms_buffer.offset;
        for (const auto& model_type : rdata.rendered_objs) {
            if (model_type.first->state != UploadState::OK) {
                continue;
            }

            rdata.thread.bindInstanceBuffer(transforms_buffer.buffer, transforms_offset);
            transforms_offset += sizeof(mat4f) * model_type.second.size();

            for (const auto& mesh : model_type.first->meshes) {
                rdata.thread.bindVertexBuffer(mesh.vertex_buffer.get());
                rdata.thread.bindIndexBuffer(mesh.index_buffer.get());

                rdata.thread.drawIndexed(as<int>(model_type.second.size()), mesh.index_count, 0);
            }
        }
    }

    rdata.thread.endRendering();
}

void SceneRenderer::renderGBuffer(RenderingData& rdata) {
    albedo_img_->transitionState(rdata.thread.cmd(), Image::State::COLOR_ATT);
    normal_img_->transitionState(rdata.thread.cmd(), Image::State::COLOR_ATT);
    material_img_->transitionState(rdata.thread.cmd(), Image::State::COLOR_ATT);
    depth_img_->transitionState(rdata.thread.cmd(), Image::State::DEPTH_ATT);

    std::vector<AttachmentInfo> attachments;
    auto& albedo_att = attachments.emplace_back();
    albedo_att.image = albedo_img_.get();
    albedo_att.load = vk::AttachmentLoadOp::eClear;
    albedo_att.store = vk::AttachmentStoreOp::eStore;
    auto& normal_att = attachments.emplace_back();
    normal_att.image = normal_img_.get();
    normal_att.load = vk::AttachmentLoadOp::eClear;
    normal_att.store = vk::AttachmentStoreOp::eStore;
    auto& material_att = attachments.emplace_back();
    material_att.image = material_img_.get();
    material_att.load = vk::AttachmentLoadOp::eClear;
    material_att.store = vk::AttachmentStoreOp::eStore;

    rdata.thread.setColorAttachments(std::move(attachments));

    AttachmentInfo depth_attachment{};
    depth_attachment.image = depth_img_.get();
    depth_attachment.load = vk::AttachmentLoadOp::eClear;
    depth_attachment.store = vk::AttachmentStoreOp::eStore;
    depth_attachment.clear_value.setDepthStencil({1.0F, 0});
    rdata.thread.setDepthAttachment(depth_attachment);

    rdata.thread.beginRendering();
    rdata.thread.setViewport();

    auto& transforms_buffer = rdata.rendered_transforms;

    if (!rdata.rendered_objs.empty()) {
        rdata.thread.setPipeline(pipelines_.vox);

        struct {
            mat4f vp;
        } pc{};

        pc.vp = rdata.view_proj;

        rdata.thread.pushConstants(&pc);

        usize transforms_offset = transforms_buffer.offset;
        for (auto& model_type : rdata.rendered_objs) {
            if (model_type.first->state != UploadState::OK) {
                continue;
            }

            rdata.thread.bindInstanceBuffer(transforms_buffer.buffer, transforms_offset);
            transforms_offset += sizeof(mat4f) * model_type.second.size();

            for (const auto& mesh : model_type.first->meshes) {
                rdata.thread.bindVertexBuffer(mesh.vertex_buffer.get());
                rdata.thread.bindIndexBuffer(mesh.index_buffer.get());

                rdata.thread.drawIndexed(as<int>(model_type.second.size()), mesh.index_count, 0);
            }
        }
    }

    if (!rdata.rendered_planes.empty()) {
        rdata.thread.setPipeline(pipelines_.plane);

        struct {
            mat4f vp;
            vec2f grid_pos;
            vec2f plane_size{CHUNK_SIZE};
            vec3f color = {0, 1, 0};
            int plane_divisions = 10;
        } pc{};
        pc.vp = rdata.view_proj;
        pc.color = rdata.dim.plane_color;
        pc.plane_divisions = CHUNK_SIZE * 10;

        for (auto& p : rdata.rendered_planes) {
            pc.grid_pos.x = as<f32>(p.x);
            pc.grid_pos.y = as<f32>(p.y);
            pc.grid_pos *= CHUNK_SIZE;

            rdata.thread.pushConstants(&pc);
            rdata.thread.draw(1, 4);
        }
    }

    rdata.thread.endRendering();
}

void SceneRenderer::renderSkybox(RenderingData& rdata) {
    if (!skybox_.ready()) {
        return;
    }

    std::vector<AttachmentInfo> attachments;
    auto& c0 = attachments.emplace_back();
    c0.image = target_img_.get();
    c0.load = vk::AttachmentLoadOp::eLoad;
    c0.store = vk::AttachmentStoreOp::eStore;
    rdata.thread.setColorAttachments(std::move(attachments));

    depth_img_->transitionState(rdata.thread.cmd(), Image::State::DEPTH_ATT);

    AttachmentInfo depth_attachment{};
    depth_attachment.image = depth_img_.get();
    depth_attachment.load = vk::AttachmentLoadOp::eLoad;
    depth_attachment.store = vk::AttachmentStoreOp::eStore;
    rdata.thread.setDepthAttachment(depth_attachment);

    rdata.thread.beginRendering({});

    rdata.thread.setViewport();

    rdata.thread.setPipeline(pipelines_.skybox);

    vk::DescriptorImageInfo ii;
    ii.setImageView(skybox_.getView());
    ii.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    ii.setSampler(getNearestSampler());

    vk::WriteDescriptorSet wds;
    wds.setImageInfo(ii);
    wds.setDstBinding(0);
    wds.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    wds.setDstArrayElement(0);
    wds.setDescriptorCount(1);

    rdata.thread.pushDescriptor(0, {wds});

    rdata.thread.bindIndexBuffer(CubeMap::getIndexBuffer());

    struct PushConstants {
        mat4f view;
        mat4f proj;
    } pc{.view = camera_.getView(), .proj = camera_.getProj()};

    rdata.thread.pushConstants(&pc);

    rdata.thread.drawIndexed(1, CubeMap::getIndexCount(), 0);
}

void SceneRenderer::renderLighting(RenderingData& rdata) {
    std::vector<vk::WriteDescriptorSet> writes;

    // layout(set = 0, binding = 0) uniform sampler2D in_albedo;
    albedo_img_->transitionState(rdata.thread.cmd(), Image::State::SHADER_READ);
    vk::DescriptorImageInfo albedo_info;
    albedo_info.setImageView(albedo_img_->getView());
    albedo_info.setImageLayout(albedo_img_->getCurrentLayout());
    albedo_info.setSampler(getNearestSampler());
    auto& b0 = writes.emplace_back();
    b0.dstBinding = 0;
    b0.dstArrayElement = 0;
    b0.descriptorCount = 1;
    b0.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b0.pImageInfo = &albedo_info;

    // layout(set = 0, binding = 1) uniform sampler2D in_normal;
    normal_img_->transitionState(rdata.thread.cmd(), Image::State::SHADER_READ);
    vk::DescriptorImageInfo normal_info;
    normal_info.setImageView(normal_img_->getView());
    normal_info.setImageLayout(normal_img_->getCurrentLayout());
    normal_info.setSampler(getNearestSampler());
    auto& b1 = writes.emplace_back();
    b1.dstBinding = 1;
    b1.dstArrayElement = 0;
    b1.descriptorCount = 1;
    b1.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b1.pImageInfo = &normal_info;

    // layout(set = 0, binding = 2) uniform sampler2D in_material;
    material_img_->transitionState(rdata.thread.cmd(), Image::State::SHADER_READ);
    vk::DescriptorImageInfo material_info;
    material_info.setImageView(material_img_->getView());
    material_info.setImageLayout(material_img_->getCurrentLayout());
    material_info.setSampler(getNearestSampler());
    auto& b2 = writes.emplace_back();
    b2.dstBinding = 2;
    b2.dstArrayElement = 0;
    b2.descriptorCount = 1;
    b2.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b2.pImageInfo = &material_info;

    // layout(set = 0, binding = 3) uniform sampler2D in_depth;
    depth_img_->transitionState(rdata.thread.cmd(), Image::State::SHADER_READ);
    vk::DescriptorImageInfo depth_info;
    depth_info.setImageView(depth_img_->getView());
    depth_info.setImageLayout(depth_img_->getCurrentLayout());
    depth_info.setSampler(getNearestSampler());
    auto& b3 = writes.emplace_back();
    b3.dstBinding = 3;
    b3.dstArrayElement = 0;
    b3.descriptorCount = 1;
    b3.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b3.pImageInfo = &depth_info;

    // layout(set = 0, binding = 4) uniform GlobalUniforms globals;
    LightingUBO globals{};
    globals.view = rdata.view;
    globals.proj = rdata.proj;
    globals.view_proj = rdata.view_proj;
    globals.inv_view_proj = glm::inverse(globals.view_proj);
    globals.sun_light_mtx = rdata.sun_vp;
    globals.sun_dir = rdata.dim.sun_dir;
    globals.sun_color = rdata.dim.sun_color;
    globals.sun_intensity = rdata.dim.sun_intensity;
    auto buffer_slice = getHostVisibleBuffer(sizeof(LightingUBO), vk::BufferUsageFlagBits::eUniformBuffer);
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
    shadow_img_->transitionState(rdata.thread.cmd(), Image::State::SHADER_READ);
    vk::DescriptorImageInfo sun_shadow_info;
    sun_shadow_info.setImageView(shadow_img_->getView());
    sun_shadow_info.setImageLayout(shadow_img_->getCurrentLayout());
    sun_shadow_info.setSampler(getShadowSampler());
    auto& b5 = writes.emplace_back();
    b5.dstBinding = 5;
    b5.dstArrayElement = 0;
    b5.descriptorCount = 1;
    b5.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b5.pImageInfo = &sun_shadow_info;

    target_img_->transitionState(rdata.thread.cmd(), Image::State::COLOR_ATT);

    std::vector<AttachmentInfo> attachments;
    auto& c0 = attachments.emplace_back();
    c0.image = target_img_.get();
    c0.load = vk::AttachmentLoadOp::eClear;
    c0.store = vk::AttachmentStoreOp::eStore;
    rdata.thread.setColorAttachments(std::move(attachments));
    rdata.thread.setDepthAttachment({});

    rdata.thread.beginRendering();

    rdata.thread.setViewport();

    rdata.thread.setPipeline(pipelines_.lighting);
    rdata.thread.pushDescriptor(0, writes);
    rdata.thread.draw(1, 3);

    rdata.thread.endRendering();
}

void SceneRenderer::calcSunCamera(RenderingData& rdata) {
    const auto& sun_dir = glm::normalize(rdata.dim.sun_dir);
    const Frustum& cam_frustum = rdata.camera_frustum;

    auto cam_frustum_corners = cam_frustum.getCorners();
    auto cam_frustum_center = Frustum::calcCenter(cam_frustum_corners);

    vec3f sun_eye = cam_frustum_center - sun_dir * 100.0F;
    vec3f sun_target = cam_frustum_center;
    vec3f up = glm::abs(glm::dot(sun_dir, vec3f{0, 1, 0})) > 0.99F ? vec3f{0, 0, 1} : vec3f{0, 1, 0};
    mat4f light_view = glm::lookAt(sun_eye, sun_target, up);

    AABB light_space_bounds;
    for (const auto& corner : cam_frustum_corners) {
        vec4f light_space_pos = light_view * vec4f(corner, 1.0F);
        light_space_bounds.expand(vec3f(light_space_pos));
    }

    light_space_bounds.addPaddin(vec3f{8});

    vec3f min = light_space_bounds.min();
    vec3f max = light_space_bounds.max();

    auto& sun_cam = rdata.sun_cam;
    sun_cam.setProjType(Camera::ProjType::ORTH);
    sun_cam.setLeft(min.x);
    sun_cam.setRight(max.x);
    sun_cam.setBottom(min.y);
    sun_cam.setTop(max.y);
    sun_cam.setNear(-max.z);
    sun_cam.setFar(-min.z);
    sun_cam.setEye(sun_eye);
    sun_cam.setTarget(sun_target);
    sun_cam.setUp(up);

    rdata.sun_frustum = sun_cam.getFrustum();
    rdata.sun_vp = sun_cam.getViewProj();
}
}  // namespace mle::renderer
