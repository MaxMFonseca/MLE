#include "ModelTest.h"

#include <algorithm>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>

#include "mle/client/Client.h"
#include "mle/core/Assert.h"
#include "mle/renderer/Animation.h"
#include "mle/renderer/Buffer.h"
#include "mle/renderer/GLTF.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/utils/File.h"
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

mat4f makeModelMatrix(const std::vector<Model::NodeMesh>& meshes) {
    vec3f min_v{+FLT_MAX, +FLT_MAX, +FLT_MAX};
    vec3f max_v{-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (const auto& node_mesh : meshes) {
        const Mesh& mesh = node_mesh.mesh;
        if (mesh.getIndexCount() == 0) {
            continue;
        }

        min_v = glm::min(min_v, mesh.min());
        max_v = glm::max(max_v, mesh.max());
    }

    if (min_v.x == +FLT_MAX || max_v.x == -FLT_MAX) {
        return mat4f{1.0F};
    }

    const vec3f center = (min_v + max_v) * 0.5F;
    const vec3f extent = max_v - min_v;
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

bool isGLTFAsset(const Path& path) {
    const std::string ext = path.extension().generic_string();
    return ext == ".glb" || ext == ".gltf";
}

std::vector<std::string> discoverAssets(const std::string& resource_dir) {
    const Path base = Path{ResPath::RES} / resource_dir / ResPath::USER_SUBDIR;
    std::vector<std::string> files;

    std::error_code ec;
    if (!std::filesystem::exists(base, ec) || !std::filesystem::is_directory(base, ec)) {
        return files;
    }

    auto entries = getEntriesInDirectory(base, true);
    if (!entries.has_value()) {
        return files;
    }

    for (const auto& path : entries.value()) {
        if (!std::filesystem::is_regular_file(path, ec) || !isGLTFAsset(path)) {
            continue;
        }

        const Path rel = path.lexically_relative(base);
        files.push_back((Path{ResPath::USER_SUBDIR} / rel).generic_string());
    }

    std::ranges::sort(files);
    return files;
}

std::string makeAnimationDisplayName(const std::string& animation_file, AnimationClipRef clip, usize animation_index) {
    if (clip != nullptr && !clip->getName().empty()) {
        return animation_file + "." + clip->getName();
    }
    return animation_file + "." + std::to_string(animation_index);
}

bool animationTargetsModel(AnimationClipRef animation, ModelRef model) {
    if (animation == nullptr || model == nullptr) {
        return false;
    }

    return std::ranges::any_of(animation->getNodes(),
                               [&model](const auto& node_anim) { return model->getNodeIdxByName(node_anim.target_name) != max<usize>(); });
}
}  // namespace

void ModelTestLayer::init() {
    MLE_I("ModelTestLayer::init()");

    refreshAssets();

    getModelTestPipeline();
    Client::i().getGameLayerTable()["model_test_set_camera_yaw"] = [this](f32 value) { setCameraYaw01(value); };
    Client::i().getGameLayerTable()["model_test_set_camera_pitch"] = [this](f32 value) { setCameraPitch01(value); };
    Client::i().getGameLayerTable()["model_test_set_model"] = [this](const std::string& name) { setModel(name); };
    Client::i().getGameLayerTable()["model_test_set_animation"] = [this](const std::string& name) { setAnimation(name); };
    Client::i().getGameLayerTable()["model_test_refresh_assets"] = [this]() { return refreshAssetsForLua(); };
    Client::i().getGameLayerTable()["model_test_model_names"] = makeModelNamesTable();
    Client::i().getGameLayerTable()["model_test_animation_names"] = makeAnimationNamesTable();
    ui_.setRoot("i/ui/ModelTestLayer");

    MLE_I("ModelTestLayer assets loaded: {} models, {} animations", model_files_.size(), animation_names_.size());
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
    if (!target || !model_ || model_->getMeshes().empty()) {
        return;
    }

    const auto& meshes = model_->getMeshes();
    if (skin_binding_.jointCount() == 0) {
        return;
    }

    if (current_animation_ != nullptr) {
        current_animation_->evaluate(*model_, animation_time_, node_globals_);
    } else {
        model_->evaluateBase(node_globals_);
    }
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

    pc.model = makeModelMatrix(meshes);
    pc.view_proj = makeViewProj(target->getExtent(), camera_yaw_, camera_pitch_);
    thread.pushConstants(&pc);

    for (const auto& node_mesh : meshes) {
        const Mesh& mesh = node_mesh.mesh;
        if (!mesh.isSkinned() || mesh.getIndexCount() == 0) {
            continue;
        }

        thread.bindVertexBuffer(mesh.getVertexBuffer());
        thread.bindIndexBuffer(mesh.getIndexBuffer());
        thread.drawIndexed(as<u32>(mesh.getIndexCount()), 1);
    }

    thread.executeCommands();
}

void ModelTestLayer::refreshAssets() {
    model_files_ = discoverAssets(ResPath::MODELS);
    animation_files_ = discoverAssets(ResPath::ANIMATIONS);

    animations_.clear();
    animation_names_.clear();

    auto& renderer = Renderer::i();
    for (const auto& animation_file : animation_files_) {
        auto refs = renderer.animationCache().addAnimations(animation_file, ResPath::ANIMATIONS);
        for (usize i = 0; i < refs.size(); ++i) {
            animations_.push_back(refs[i]);
            animation_names_.push_back(makeAnimationDisplayName(animation_file, refs[i], i));
        }
    }

    if (current_model_name_.empty() || std::ranges::find(model_files_, current_model_name_) == model_files_.end()) {
        model_ = nullptr;
        skin_binding_ = SkinBinding{};
        current_model_name_.clear();
        if (!model_files_.empty()) {
            setModel(model_files_.front());
        }
    } else {
        setModel(current_model_name_);
    }

    if (current_animation_name_.empty() || std::ranges::find(animation_names_, current_animation_name_) == animation_names_.end()) {
        current_animation_ = nullptr;
        current_animation_name_.clear();
        if (!animation_names_.empty()) {
            setAnimation(animation_names_.front());
        }
    } else {
        setAnimation(current_animation_name_);
    }
}

sol::table ModelTestLayer::refreshAssetsForLua() {
    refreshAssets();

    auto ret = Client::i().lua().createTable();
    ret["models"] = makeModelNamesTable();
    ret["animations"] = makeAnimationNamesTable();
    return ret;
}

sol::table ModelTestLayer::makeModelNamesTable() const {
    auto table = Client::i().lua().createTable();
    for (usize i = 0; i < model_files_.size(); ++i) {
        table[i + 1] = model_files_[i];
    }
    return table;
}

sol::table ModelTestLayer::makeAnimationNamesTable() const {
    auto table = Client::i().lua().createTable();
    for (usize i = 0; i < animation_names_.size(); ++i) {
        table[i + 1] = animation_names_[i];
    }
    return table;
}

bool ModelTestLayer::setModel(const std::string& name) {
    if (std::ranges::find(model_files_, name) == model_files_.end()) {
        MLE_W("ModelTestLayer model '{}' was not found", name);
        return false;
    }

    auto& model_cache = Renderer::i().modelCache();
    ModelRef model = model_cache.getModel(name);
    if (model == nullptr) {
        model = model_cache.addModel(name);
    }
    if (model == nullptr) {
        return false;
    }

    GLTF model_gltf;
    const Path model_path = Path{ResPath::RES} / ResPath::MODELS / name;
    if (model_gltf.load(model_path) != Result::OK) {
        MLE_W("ModelTestLayer failed to load GLTF for skin binding '{}'", model_path.generic_string());
        return false;
    }

    model_ = model;
    current_model_name_ = name;
    node_globals_.clear();
    skin_mats_.clear();
    skin_binding_ = SkinBinding{};
    animation_time_ = 0.0F;

    if (model_gltf.model().skins.empty()) {
        MLE_W("ModelTestLayer model '{}' has no skin; skinned preview will be skipped", name);
        return true;
    }

    skin_binding_.loadFromGLTF(model_gltf);

    if (current_animation_ != nullptr && !animationTargetsModel(current_animation_, model_)) {
        MLE_W("ModelTestLayer animation '{}' has no channels matching model '{}'; base pose will be used", current_animation_name_, current_model_name_);
    }

    return true;
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

void ModelTestLayer::setAnimation(const std::string& name) {
    for (usize i = 0; i < animations_.size(); ++i) {
        if (animation_names_.at(i) == name) {
            current_animation_ = animations_[i];
            current_animation_name_ = name;
            animation_time_ = 0.0F;
            if (model_ != nullptr && !animationTargetsModel(current_animation_, model_)) {
                MLE_W("ModelTestLayer animation '{}' has no channels matching model '{}'; base pose will be used", current_animation_name_,
                      current_model_name_);
            }
            return;
        }
    }

    MLE_W("ModelTestLayer animation '{}' was not found", name);
}
}  // namespace mle::user
