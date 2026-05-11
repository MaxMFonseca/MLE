#include "ModelTest.h"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

#include "mle/client/Client.h"
#include "mle/core/Assert.h"
#include "mle/renderer/Animation.h"
#include "mle/renderer/Buffer.h"
#include "mle/renderer/GLTF.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/window/Window.h"

namespace mle::user {
namespace {
const Pipeline* getModelTestPipeline() {
    static const Pipeline* pipeline = nullptr;
    if (pipeline == nullptr) {
        Pipeline::CI pipeline_ci{};
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/model_test/skinned.vert");
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/model_test/color.frag");
        std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::COLOR)};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
        pipeline_ci.blend_attachments = blend_attachments;
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleList;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        pipeline_ci.depth = true;
        pipeline_ci.depth_write = true;
        pipeline_ci.push_descriptor = 0;

        pipeline = &Renderer::i().pipelineCache().setPipeline("model_test_skinned", pipeline_ci);
    }
    return pipeline;
}

mat4f makeModelMatrix(const Mesh& mesh) {
    const vec3f center = (mesh.min() + mesh.max()) * 0.5F;
    const vec3f extent = mesh.max() - mesh.min();
    const f32 radius = glm::length(extent) * 0.5F;
    const f32 scale = radius > 0.0F ? 1.45F / radius : 1.0F;

    return glm::scale(mat4f{1.0F}, vec3f{scale}) * glm::translate(mat4f{1.0F}, -center);
}

mat4f makeViewProj(vec2u extent, f32 yaw, f32 pitch) {
    constexpr f32 CAMERA_DISTANCE = 3.0F;
    const f32 aspect = extent.y > 0 ? as<f32>(extent.x) / as<f32>(extent.y) : 1.0F;
    const vec3f target{0.0F, 0.15F, 0.0F};
    const f32 pitch_cos = std::cos(pitch);
    const vec3f orbit_dir{
        std::sin(yaw) * pitch_cos,
        std::sin(pitch),
        std::cos(yaw) * pitch_cos,
    };
    mat4f view = glm::lookAt(target + orbit_dir * CAMERA_DISTANCE, target, vec3f{0.0F, 1.0F, 0.0F});
    mat4f proj = glm::perspective(glm::radians(45.0F), aspect, 0.01F, 100.0F);
    proj[1][1] *= -1.0F;
    return proj * view;
}
}  // namespace

void ModelTestLayer::init() {
    MLE_I("ModelTestLayer::init()");

    constexpr auto TEST_MODEL_PATH = "i/Char1/BaseChar1.gltf";
    constexpr auto ARMATURE_PATH = "i/HumanoidArmature/Humanoid.gltf";
    constexpr auto IDLE_ANIMATION_NAME = "i/HumanoidArmature/Humanoid.gltf.Idle";

    auto& renderer = Renderer::i();

    model_ = renderer.modelCache().addModel(TEST_MODEL_PATH);
    MLE_ASSERT_LOG(model_ != nullptr, "Failed to load test model '{}'", TEST_MODEL_PATH);
    MLE_ASSERT_LOG(model_->getNodeCount() == 27, "Unexpected test model node count: {}", model_->getNodeCount());
    MLE_ASSERT_LOG(model_->getMeshes().size() == 1, "Unexpected test model mesh count: {}", model_->getMeshes().size());
    MLE_ASSERT_LOG(model_->getNodeIdxByName("Armature") != max<usize>(), "Test model Armature node not found");

    const auto& node_mesh = model_->getMeshes().front();
    MLE_ASSERT_LOG(node_mesh.node_index == model_->getNodeIdxByName("_FTemplate"), "Test model mesh is not attached to _FTemplate");
    MLE_ASSERT_LOG(node_mesh.mesh.getIndexCount() > 0, "Test model mesh has no indices");
    MLE_ASSERT_LOG(node_mesh.mesh.isSkinned(), "Test model mesh should be skinned");
    MLE_ASSERT_LOG(node_mesh.mesh.getColorMultiplierName().empty(), "Test model unexpectedly has a color_multiplier extra");

    skeleton_ = renderer.skeletonCache().addSkeleton(ARMATURE_PATH);
    MLE_ASSERT_LOG(skeleton_ != nullptr, "Failed to load reusable skeleton '{}'", ARMATURE_PATH);
    MLE_ASSERT_LOG(skeleton_->jointCount() == 25, "Unexpected skeleton joint count: {}", skeleton_->jointCount());
    MLE_ASSERT_LOG(!skeleton_->getJoints().empty(), "Skeleton has no joints");

    std::vector<AnimationClipRef> animations = renderer.animationCache().addAnimations(ARMATURE_PATH);
    MLE_ASSERT_LOG(animations.size() == 18, "Unexpected animation count: {}", animations.size());

    idle_ = renderer.animationCache().getAnimation(IDLE_ANIMATION_NAME);
    MLE_ASSERT_LOG(idle_ != nullptr, "Idle animation '{}' was not cached", IDLE_ANIMATION_NAME);
    MLE_ASSERT_LOG(idle_->getDuration() > 0.0F, "Idle animation has invalid duration");
    MLE_ASSERT_LOG(!idle_->getNodes().empty(), "Idle animation has no node channels");

    std::vector<mat4f> base_globals;
    model_->evaluateBase(base_globals);
    MLE_ASSERT_LOG(base_globals.size() == model_->getNodeCount(), "Base evaluation produced wrong node global count");

    std::vector<mat4f> animated_globals;
    idle_->evaluate(*model_, 0.25F, animated_globals);
    MLE_ASSERT_LOG(animated_globals.size() == model_->getNodeCount(), "Animation evaluation produced wrong node global count");

    GLTF model_gltf;
    MLE_ASSERT_LOG(model_gltf.load(Path{"res"} / "models" / TEST_MODEL_PATH) == Result::OK, "Failed to load GLTF for skin binding");

    skin_binding_.loadFromGLTF(model_gltf);
    MLE_ASSERT_LOG(skin_binding_.jointCount() == skeleton_->jointCount(), "Skin binding/skeleton joint count mismatch");
    MLE_ASSERT_LOG(skin_binding_.getJoints().front().name == skeleton_->getJoints().front().name, "Skin binding/skeleton first joint mismatch");

    std::vector<mat4f> skin_mats;
    skin_binding_.buildSkinMatrices(animated_globals, skin_mats);
    MLE_ASSERT_LOG(skin_mats.size() == skeleton_->jointCount(), "Skin matrix count mismatch");

    getModelTestPipeline();
    Client::i().getGameLayerTable()["model_test_set_camera_yaw"] = [this](f32 value) { setCameraYaw01(value); };
    Client::i().getGameLayerTable()["model_test_set_camera_pitch"] = [this](f32 value) { setCameraPitch01(value); };
    ui_.setRoot("i/ui/ModelTestLayer");

    MLE_I("ModelTestLayer model/skeleton/animation/skin-binding checks passed");
};

void ModelTestLayer::update() {
    animation_time_ += 1.0F / 60.0F;
    ui_.update();
};

ImageRef ModelTestLayer::render() {
    auto* image = getImage();

    renderModel(image);

    if (auto* ui_image = ui_.render(); ui_image != nullptr) {
        image->blend(Renderer::i().frameRenderer().cmd(), *ui_image);
    }

    return image;
};

void ModelTestLayer::shutdown() {
    ui_.shutdown();
};

ImageRef ModelTestLayer::getImage() {
    auto& frame_renderer = Renderer::i().frameRenderer();
    auto frame_idx = frame_renderer.getCurrentFrameId();
    auto& image = images_.at(frame_idx);
    vec2u size = Window::i().getSize();

    if (!image) {
        Image::CI image_ci{};
        image_ci.extent = size;
        image_ci.format = Image::Format::COLOR;
        image_ci.extra_usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
        image = Image::createHnd(image_ci);
    } else if (image->getExtent() != size) {
        frame_renderer.deleteAfterFrame(std::move(image));
        Image::CI image_ci{};
        image_ci.extent = size;
        image_ci.format = Image::Format::COLOR;
        image_ci.extra_usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
        image = Image::createHnd(image_ci);
    }

    image->clear(Renderer::i().frameRenderer().cmd());

    return image.get();
};

ImageRef ModelTestLayer::getDepthImage(vec2u size) {
    auto& frame_renderer = Renderer::i().frameRenderer();
    auto frame_idx = frame_renderer.getCurrentFrameId();
    auto& image = depth_images_.at(frame_idx);

    if (!image) {
        Image::CI image_ci{};
        image_ci.extent = size;
        image_ci.format = Image::Format::DEPTH;
        image = Image::createHnd(image_ci);
    } else if (image->getExtent() != size) {
        frame_renderer.deleteAfterFrame(std::move(image));
        Image::CI image_ci{};
        image_ci.extent = size;
        image_ci.format = Image::Format::DEPTH;
        image = Image::createHnd(image_ci);
    }

    return image.get();
}

void ModelTestLayer::renderModel(ImageRef target) {
    if (!target || !model_ || !idle_) {
        return;
    }

    const auto& node_mesh = model_->getMeshes().front();
    const Mesh& mesh = node_mesh.mesh;

    idle_->evaluate(*model_, animation_time_, node_globals_);
    skin_binding_.buildSkinMatrices(node_globals_, skin_mats_);
    if (skin_mats_.empty()) {
        return;
    }

    auto& renderer = Renderer::i();
    auto& frame_renderer = renderer.frameRenderer();

    RenderingThread thread;
    thread.init();

    BufferSlice skin_mats_slice = frame_renderer.getHostVisibleBuffer(sizeof(mat4f) * skin_mats_.size(), vk::BufferUsageFlagBits::eStorageBuffer);
    skin_mats_slice.buffer->write(skin_mats_.data(), skin_mats_slice.size, skin_mats_slice.offset);

    AttachmentInfo color_attachment{};
    color_attachment.image = target;
    color_attachment.load_op = vk::AttachmentLoadOp::eLoad;
    thread.setColorAttachment(color_attachment, 0);

    AttachmentInfo depth_attachment{};
    depth_attachment.image = getDepthImage(target->getExtent());
    depth_attachment.load_op = vk::AttachmentLoadOp::eClear;
    depth_attachment.clear_value.depthStencil = vk::ClearDepthStencilValue{1.0F, 0};
    thread.setDepthAttachment(depth_attachment);

    thread.beginRendering();
    thread.setViewportAndScissor(Rectf{0.0F, 0.0F, as<f32>(target->getExtent().x), as<f32>(target->getExtent().y)});

    const Pipeline* pipeline = getModelTestPipeline();
    thread.setPipeline(pipeline);

    vk::DescriptorBufferInfo skin_mats_di = skin_mats_slice.buffer->makeDescriptorInfo(thread.cmd(), skin_mats_slice.size, skin_mats_slice.offset);
    auto push_writes = pipeline->makeWrites(0, nullptr, &skin_mats_di);
    thread.pushDescriptor(0, push_writes);

    struct PushConstants {
        mat4f model;
        mat4f view_proj;
    } pc{};

    pc.model = makeModelMatrix(mesh);
    pc.view_proj = makeViewProj(target->getExtent(), camera_yaw_, camera_pitch_);
    thread.pushConstants(&pc);

    thread.bindVertexBuffer(mesh.getVertexBuffer());
    thread.bindIndexBuffer(mesh.getIndexBuffer());
    thread.drawIndexed(as<u32>(mesh.getIndexCount()), 1);

    thread.executeCommands();
}

void ModelTestLayer::setCameraYaw01(f32 value) {
    constexpr f32 TWO_PI = glm::radians(360.0F);
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    camera_yaw_ = (clamped - 0.5F) * TWO_PI;
}

void ModelTestLayer::setCameraPitch01(f32 value) {
    constexpr f32 MAX_PITCH = glm::radians(70.0F);
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    camera_pitch_ = (clamped - 0.5F) * 2.0F * MAX_PITCH;
}
}  // namespace mle::user
