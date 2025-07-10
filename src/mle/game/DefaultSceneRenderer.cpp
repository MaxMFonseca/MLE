#include "DefaultSceneRenderer.h"

#include <algorithm>
#include <cstddef>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "mle/common/ECS.h"
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
#include "mle/window/Window.h"

namespace mle::game {
void DefaultSceneRenderer::shutdown() {
    MLE_D("Shutting down scene renderer");

    renderer::detail::waitIdle();

    pipelines_ = {};
}

void DefaultSceneRenderer::init(ServerOutED& server_out_ed) {
    camera_.setProjType(renderer::Camera::ProjType::PERSPECTIVE);
    camera_.setAspect(window::getAspectRatio());
    camera_.setFov(60.0F);
    camera_.setNear(1.F);
    camera_.setFar(400.0F);
    camera_.setEye({0.0F, 5.0F, -5.0F});
    camera_.setTarget({0.0F, 0.0F, 0.0F});
    camera_.setUp({0, 1, 0});

    initSun();
    initG();
    initPlane();
    initSun();
    initLighting();
    // initSkybox(); using fog for now
    // initDebug();

    server_time_listener_ = server_out_ed.makeEventListener<server_out_events::Time>([this](const server_out_events::Time& e) { time_ = e.time_s; });

    server_new_entt_listener_ = server_out_ed.makeEventListener<server_out_events::NewEntt>([this](const server_out_events::NewEntt& e) {
        auto ent = reg_.create(e.e);
        MLE_ASSERT_LOG(ent == e.e, "Entity handle mismatch: created {} but got {}", ent, e.e);  // NOLINT
    });

    server_entt_transform_listener_ = server_out_ed.makeEventListener<server_out_events::EnttTransform>([this](const server_out_events::EnttTransform& e) {
        auto ent = e.e;
        if (!reg_.valid(ent)) {
            MLE_W("Entity {} is not valid, skipping transform update", ent);  // NOLINT
            return;
        }

        auto* tc = reg_.try_get<TransformComp>(ent);
        if (!tc) {
            tc = &reg_.emplace<TransformComp>(ent);
            tc->pos = e.pos;
            tc->scale = e.scale;
            tc->rot = e.rot;
            tc->time = 0;
        } else {
            auto* t_tc = reg_.try_get<TargetTransformComp>(ent);
            if (t_tc) {
                *tc = *t_tc;
            } else {
                t_tc = &reg_.emplace<TargetTransformComp>(ent);
            }

            t_tc->pos = e.pos;
            t_tc->scale = e.scale;
            t_tc->rot = e.rot;
            t_tc->time = time_;

            tc->time = last_time_;
        }
    });

    server_entt_model_listener_ = server_out_ed.makeEventListener<server_out_events::EnttModel>([this](const server_out_events::EnttModel& e) {
        auto ent = e.e;
        if (!reg_.valid(ent)) {
            MLE_W("Entity {} is not valid, skipping model update", ent);  // NOLINT
            return;
        }

        auto& mc = reg_.emplace_or_replace<ModelComp>(ent);
        mc.v = renderer::getModel(e.model_string);  // FIXME: this is a model string
    });

    MLE_D("Scene renderer initialized");
}

void DefaultSceneRenderer::initSun() {
    MLE_D("Creating sun data");

    renderer::Image::CI image_ci{};
    image_ci.extent = {4096, 4096};
    image_ci.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
    image_ci.format = renderer::getDepthFormat();
    shadow_img_ = renderer::Image::createHnd(image_ci);

    pipelines_.sun = renderer::getPipeline("mle/scene/sun");
}

// void DefaultSceneRenderer::initSkybox() {
//     if (ci.skybox.empty()) {
//         return;
//     }
//
//     MLE_D("Creating cube map");
//
//     pipelines_.skybox = renderer::getPipeline("mle/scene/cube_map");
//     skybox_.init(ci.skybox);
// }

void DefaultSceneRenderer::initG() {
    MLE_D("Creating G-Buffer pipeline");

    pipelines_.vox = renderer::getPipeline("mle/scene/vox");

    vec2i image_extent = window::getSize();

    renderer::Image::CI image_ci{};
    image_ci.extent = image_extent;
    image_ci.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
    image_ci.format = renderer::getDefaultColorFormat();
    albedo_img_ = renderer::Image::createHnd(image_ci);
    normal_img_ = renderer::Image::createHnd(image_ci);
    material_img_ = renderer::Image::createHnd(image_ci);
    image_ci.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
    image_ci.format = renderer::getDepthFormat();
    depth_img_ = renderer::Image::createHnd(image_ci);
}

void DefaultSceneRenderer::initPlane() {
    MLE_D("Creating plane pipeline");

    pipelines_.plane = renderer::getPipeline("mle/scene/plane");
}

void DefaultSceneRenderer::initLighting() {
    MLE_D("Creating lighting pipeline");

    vec2i image_extent = window::getSize();

    renderer::Image::CI image_ci{};
    image_ci.extent = image_extent;
    image_ci.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
    image_ci.format = renderer::getDefaultColorFormat();
    target_img_ = renderer::Image::createHnd(image_ci);

    pipelines_.lighting = renderer::getPipeline("mle/scene/lighting");
}

void DefaultSceneRenderer::render(f32 dt) {
    RenderingData rdata{};

    rdata.thread.init();

    rdata.dt = dt;

    rdata.view = camera_.getView();
    rdata.proj = camera_.getProj();
    rdata.view_proj = camera_.getViewProj();
    rdata.inv_view_proj = camera_.getInvViewProj();
    rdata.camera_frustum = camera_.getFrustum();

    auto camera_frustum_inter_y0 = rdata.camera_frustum.intersectWithY0();
    rdata.camera_center_y0 = camera_frustum_inter_y0.center();
    rdata.camera_frustum_inter_y0 = camera_frustum_inter_y0.xz();

    updateEntities(rdata);

    renderSun(rdata);
    renderGBuffer(rdata);
    renderLighting(rdata);
    // renderSkybox(rdata);
    // renderDebug(rdata);

    rdata.thread.submit();
}

void DefaultSceneRenderer::updateEntities(RenderingData& rdata) {
    auto dt = rdata.dt;

    auto transform_view = reg_.view<TargetTransformComp, TransformComp>();
    for (auto e : transform_view) {
        auto& t_tc = transform_view.get<TargetTransformComp>(e);
        auto& tc = transform_view.get<TransformComp>(e);

        if (tc.time + dt >= t_tc.time) {
            tc.pos = t_tc.pos;
            tc.rot = t_tc.rot;
            tc.scale = t_tc.scale;
            tc.time = 0.0F;
            reg_.remove<TargetTransformComp>(e);
            MLE_VC("Here");
            continue;
        }

        f32 alpha = std::clamp(dt / (t_tc.time - tc.time), 0.0F, 1.0F);
        tc.time += dt;

        tc.pos = glm::mix(tc.pos, t_tc.pos, alpha);
        tc.scale = glm::mix(tc.scale, t_tc.scale, alpha);
        tc.rot = glm::slerp(tc.rot, t_tc.rot, alpha);
    }
}

void DefaultSceneRenderer::renderSun(RenderingData& rdata) {  // NOLINT
    calcSunCamera(rdata);

    auto sun_frustum_inter_y0 = rdata.sun_frustum.intersectWithY0();
    Polygon2f sun_frustum_polygon(sun_frustum_inter_y0.xz());
    AABB2D sun_frustum_aabb = sun_frustum_polygon.boundingBox();

    int sun_aabb_inter_chunk_range_x0 = as<int>(std::floor(sun_frustum_aabb.min().x / CHUNK_SIZE));
    int sun_aabb_inter_chunk_range_x1 = as<int>(std::ceil(sun_frustum_aabb.max().x / CHUNK_SIZE));
    int sun_aabb_inter_chunk_range_z0 = as<int>(std::floor(sun_frustum_aabb.min().y / CHUNK_SIZE));
    int sun_aabb_inter_chunk_range_z1 = as<int>(std::ceil(sun_frustum_aabb.max().y / CHUNK_SIZE));

    usize obj_count = 0;

    for (int x_idx = sun_aabb_inter_chunk_range_x0; x_idx <= sun_aabb_inter_chunk_range_x1; ++x_idx) {
        for (int z_idx = sun_aabb_inter_chunk_range_z0; z_idx <= sun_aabb_inter_chunk_range_z1; ++z_idx) {
            int x1 = x_idx * as<int>(CHUNK_SIZE);
            int x2 = x1 + as<int>(CHUNK_SIZE);
            int z1 = z_idx * as<int>(CHUNK_SIZE);
            int z2 = z1 + as<int>(CHUNK_SIZE);

            AABB2D chunk_aabb{std::array{vec2f{x1, z1}, vec2f{x2, z2}}};

            if (chunk_aabb.intersects(sun_frustum_aabb)) {
                vec2f chunk_base_pos{x1, z1};

                rdata.rendered_planes.emplace_back(chunk_base_pos);
            }
        }
    }

    auto transform_model_view = reg_.view<TransformComp, ModelComp>();

    for (auto e : transform_model_view) {
        const auto& model_c = transform_model_view.get<ModelComp>(e);
        if (model_c.v->state != renderer::UploadState::OK) {
            continue;
        }

        const auto& tc = transform_model_view.get<TransformComp>(e);

        // This is the sun cull and it does have a little of padding on the borders so instead of transforming the model
        // we will cull it based on position only
        if (!sun_frustum_aabb.contains({tc.pos.x, tc.pos.z})) {
            continue;
        }

        mat4f obj_transform = glm::translate(mat4f{1}, tc.pos) * glm::toMat4(tc.rot) * glm::scale(mat4f{1}, tc.scale);
        rdata.rendered_objs[model_c.v].emplace_back(obj_transform);
        obj_count++;
    }

    rdata.rendered_transforms = renderer::getHostVisibleBuffer(obj_count * sizeof(mat4f), vk::BufferUsageFlagBits::eVertexBuffer);
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

    renderer::AttachmentInfo sun_depth_attachment{};
    sun_depth_attachment.image = shadow_img_.get();
    sun_depth_attachment.load = vk::AttachmentLoadOp::eClear;
    sun_depth_attachment.store = vk::AttachmentStoreOp::eStore;
    sun_depth_attachment.clear_value.setDepthStencil({1.0F, 0});
    shadow_img_->transitionState(rdata.thread.cmd(), renderer::Image::State::DEPTH_ATT);
    rdata.thread.setDepthAttachment(sun_depth_attachment);
    rdata.thread.setColorAttachments({});

    rdata.thread.beginRendering({});

    rdata.thread.setViewport();

    MLE_ASSERT(!rdata.rendered_objs.empty());

    if (!rdata.rendered_objs.empty()) {
        rdata.thread.setPipeline(pipelines_.sun);
        struct {
            mat4f vp;
        } pc{};

        pc.vp = rdata.sun_vp;

        rdata.thread.pushConstants(&pc);

        usize transforms_offset = transforms_buffer.offset;
        for (const auto& model_type : rdata.rendered_objs) {
            if (model_type.first->state != renderer::UploadState::OK) {
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

void DefaultSceneRenderer::renderGBuffer(RenderingData& rdata) {
    albedo_img_->transitionState(rdata.thread.cmd(), renderer::Image::State::COLOR_ATT);
    normal_img_->transitionState(rdata.thread.cmd(), renderer::Image::State::COLOR_ATT);
    material_img_->transitionState(rdata.thread.cmd(), renderer::Image::State::COLOR_ATT);
    depth_img_->transitionState(rdata.thread.cmd(), renderer::Image::State::DEPTH_ATT);

    std::vector<renderer::AttachmentInfo> attachments;
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

    renderer::AttachmentInfo depth_attachment{};
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
            if (model_type.first->state != renderer::UploadState::OK) {
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
        pc.color = floor_color_;
        pc.plane_divisions = CHUNK_SIZE * 10;

        for (auto& p : rdata.rendered_planes) {
            pc.grid_pos.x = as<f32>(p.x);
            pc.grid_pos.y = as<f32>(p.y);

            rdata.thread.pushConstants(&pc);
            rdata.thread.draw(1, 4);
        }
    }

    rdata.thread.endRendering();
}

// void DefaultSceneRenderer::renderSkybox(RenderingData& rdata) {
//     if (!skybox_.ready()) {
//         return;
//     }
//
//     std::vector<renderer::AttachmentInfo> attachments;
//     auto& c0 = attachments.emplace_back();
//     c0.image = target_img_.get();
//     c0.load = vk::AttachmentLoadOp::eLoad;
//     c0.store = vk::AttachmentStoreOp::eStore;
//     rdata.thread.setColorAttachments(std::move(attachments));
//
//     depth_img_->transitionState(rdata.thread.cmd(), renderer::Image::State::DEPTH_ATT);
//
//     renderer::AttachmentInfo depth_attachment{};
//     depth_attachment.image = depth_img_.get();
//     depth_attachment.load = vk::AttachmentLoadOp::eLoad;
//     depth_attachment.store = vk::AttachmentStoreOp::eStore;
//     rdata.thread.setDepthAttachment(depth_attachment);
//
//     rdata.thread.beginRendering({});
//
//     rdata.thread.setViewport();
//
//     rdata.thread.setPipeline(pipelines_.skybox);
//
//     vk::DescriptorImageInfo ii;
//     ii.setImageView(skybox_.getView());
//     ii.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
//     ii.setSampler(renderer::getNearestSampler());
//
//     vk::WriteDescriptorSet wds;
//     wds.setImageInfo(ii);
//     wds.setDstBinding(0);
//     wds.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
//     wds.setDstArrayElement(0);
//     wds.setDescriptorCount(1);
//
//     rdata.thread.pushDescriptor(0, {wds});
//
//     rdata.thread.bindIndexBuffer(renderer::CubeMap::getIndexBuffer());
//
//     struct PushConstants {
//         mat4f view;
//         mat4f proj;
//     } pc{.view = camera_.getView(), .proj = camera_.getProj()};
//
//     rdata.thread.pushConstants(&pc);
//
//     rdata.thread.drawIndexed(1, renderer::CubeMap::getIndexCount(), 0);
// }

void DefaultSceneRenderer::renderLighting(RenderingData& rdata) {
    std::vector<vk::WriteDescriptorSet> writes;

    // layout(set = 0, binding = 0) uniform sampler2D in_albedo;
    albedo_img_->transitionState(rdata.thread.cmd(), renderer::Image::State::SHADER_READ);
    vk::DescriptorImageInfo albedo_info;
    albedo_info.setImageView(albedo_img_->getView());
    albedo_info.setImageLayout(albedo_img_->getCurrentLayout());
    albedo_info.setSampler(renderer::getNearestSampler());
    auto& b0 = writes.emplace_back();
    b0.dstBinding = 0;
    b0.dstArrayElement = 0;
    b0.descriptorCount = 1;
    b0.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b0.pImageInfo = &albedo_info;

    // layout(set = 0, binding = 1) uniform sampler2D in_normal;
    normal_img_->transitionState(rdata.thread.cmd(), renderer::Image::State::SHADER_READ);
    vk::DescriptorImageInfo normal_info;
    normal_info.setImageView(normal_img_->getView());
    normal_info.setImageLayout(normal_img_->getCurrentLayout());
    normal_info.setSampler(renderer::getNearestSampler());
    auto& b1 = writes.emplace_back();
    b1.dstBinding = 1;
    b1.dstArrayElement = 0;
    b1.descriptorCount = 1;
    b1.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b1.pImageInfo = &normal_info;

    // layout(set = 0, binding = 2) uniform sampler2D in_material;
    material_img_->transitionState(rdata.thread.cmd(), renderer::Image::State::SHADER_READ);
    vk::DescriptorImageInfo material_info;
    material_info.setImageView(material_img_->getView());
    material_info.setImageLayout(material_img_->getCurrentLayout());
    material_info.setSampler(renderer::getNearestSampler());
    auto& b2 = writes.emplace_back();
    b2.dstBinding = 2;
    b2.dstArrayElement = 0;
    b2.descriptorCount = 1;
    b2.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b2.pImageInfo = &material_info;

    // layout(set = 0, binding = 3) uniform sampler2D in_depth;
    depth_img_->transitionState(rdata.thread.cmd(), renderer::Image::State::SHADER_READ);
    vk::DescriptorImageInfo depth_info;
    depth_info.setImageView(depth_img_->getView());
    depth_info.setImageLayout(depth_img_->getCurrentLayout());
    depth_info.setSampler(renderer::getNearestSampler());
    auto& b3 = writes.emplace_back();
    b3.dstBinding = 3;
    b3.dstArrayElement = 0;
    b3.descriptorCount = 1;
    b3.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b3.pImageInfo = &depth_info;

    // layout(set = 0, binding = 4) uniform GlobalUniforms globals;

    struct LightingUBO {
        mat4f view;
        mat4f proj;
        mat4f view_proj;
        mat4f inv_view_proj;
        mat4f sun_light_mtx;
        vec3f sun_dir;
        f32 pad0{0.123};
        vec3f sun_color;
        f32 pad1{0.123F};
        vec3f fog_color{0.01, 0.02, 0.03};
        f32 pad2{0.123F};
        f32 sun_intensity{1.0F};
        f32 fog_start{10.0F};
        f32 fog_density{0.1F};
    } globals{};

    globals.view = rdata.view;
    globals.proj = rdata.proj;
    globals.view_proj = rdata.view_proj;
    globals.inv_view_proj = glm::inverse(globals.view_proj);
    globals.sun_light_mtx = rdata.sun_vp;
    globals.sun_dir = sun_dir_;
    globals.sun_color = sun_color_;
    globals.sun_intensity = sun_intensity_;
    globals.fog_color = fog_color_;
    globals.fog_start = fog_start_;
    globals.fog_density = fog_density_;
    auto buffer_slice = renderer::getHostVisibleBuffer(sizeof(LightingUBO), vk::BufferUsageFlagBits::eUniformBuffer);
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
    shadow_img_->transitionState(rdata.thread.cmd(), renderer::Image::State::SHADER_READ);
    vk::DescriptorImageInfo sun_shadow_info;
    sun_shadow_info.setImageView(shadow_img_->getView());
    sun_shadow_info.setImageLayout(shadow_img_->getCurrentLayout());
    sun_shadow_info.setSampler(renderer::getShadowSampler());
    auto& b5 = writes.emplace_back();
    b5.dstBinding = 5;
    b5.dstArrayElement = 0;
    b5.descriptorCount = 1;
    b5.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    b5.pImageInfo = &sun_shadow_info;

    target_img_->transitionState(rdata.thread.cmd(), renderer::Image::State::COLOR_ATT);

    std::vector<renderer::AttachmentInfo> attachments;
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

void DefaultSceneRenderer::calcSunCamera(RenderingData& rdata) {
    MLE_ASSERT_LOG(feq(glm::length(sun_dir_), 1.0F, 1e-6F), "Sun direction must be normalized, got: {} {}", sun_dir_, glm::length(sun_dir_));

    const Frustum& cam_frustum = rdata.camera_frustum;

    auto cam_frustum_corners = cam_frustum.getCorners();
    auto cam_frustum_center = Frustum::calcCenter(cam_frustum_corners);

    vec3f sun_eye = cam_frustum_center - sun_dir_ * 100.0F;
    vec3f sun_target = cam_frustum_center;
    vec3f up = glm::abs(glm::dot(sun_dir_, vec3f{0, 1, 0})) > 0.99F ? vec3f{0, 0, 1} : vec3f{0, 1, 0};
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
    sun_cam.setProjType(renderer::Camera::ProjType::ORTH);
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

void DefaultSceneRenderer::cameraFollow(entt::entity e, vec3f offset) {
    if (!reg_.valid(e)) {
        MLE_W("Entity {} is not valid, skipping camera follow", e);  // NOLINT
        return;
    }

    auto* tc = reg_.try_get<TransformComp>(e);
    if (!tc) {
        MLE_W("Entity {} does not have a TransformComp, skipping camera follow", e);  // NOLINT
        return;
    }

    camera_.setEye(tc->pos + offset);
    camera_.setTarget(tc->pos);
}
}  // namespace mle::game
