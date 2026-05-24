#include "ModelTest.h"

#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <string_view>
#include <unordered_set>

#include "mle/client/Client.h"
#include "mle/core/Assert.h"
#include "mle/renderer/Animation.h"
#include "mle/renderer/Buffer.h"
#include "mle/renderer/GLTF.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/utils/File.h"
#include "mle/utils/ECS.h"

namespace mle::user {
namespace {
struct MaterialUniform {
    vec4f base_color_factor;
    vec4f emissive_factor;
    vec4f pbr_factors;
};

struct LightingUniform {
    vec4f sun_direction_intensity;
    vec4f sun_color_ambient;
};

constexpr std::array<std::string_view, as<usize>(ModelTestShaderMode::COUNT)> SHADER_MODE_NAMES = {
    "PBR",
    "Cartoon",
    "Wireframe",
    "Normals",
    "Albedo",
};

const Pipeline* getModelTestPipeline(Mesh::VertexKind kind, ModelTestShaderMode mode) {
    static std::array<const Pipeline*, 4 * as<usize>(ModelTestShaderMode::COUNT)> pipelines{};
    const usize kind_idx = as<usize>(kind);
    const usize mode_idx = as<usize>(mode);
    const usize idx = mode_idx * 4 + kind_idx;
    if (pipelines[idx] == nullptr) {
        Pipeline::CI pipeline_ci{};
        std::string_view mode_name = "pbr";
        switch (kind) {
            case Mesh::VertexKind::PBR_COLOR:
                pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/model_pbr/color.vert");
                break;
            case Mesh::VertexKind::PBR_COLOR_SKINNED:
                pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/model_pbr/color_skinned.vert");
                break;
            case Mesh::VertexKind::PBR_TEXTURE:
                pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/model_pbr/texture.vert");
                break;
            case Mesh::VertexKind::PBR_TEXTURE_SKINNED:
                pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/model_pbr/texture_skinned.vert");
                break;
        }

        const bool textured = kind == Mesh::VertexKind::PBR_TEXTURE || kind == Mesh::VertexKind::PBR_TEXTURE_SKINNED;
        switch (mode) {
            case ModelTestShaderMode::PBR:
                mode_name = "pbr";
                pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get(textured ? "mle/model_pbr/texture.frag" : "mle/model_pbr/color.frag");
                break;
            case ModelTestShaderMode::CARTOON:
                mode_name = "cartoon";
                pipeline_ci.fragment_shader =
                    &Renderer::i().shaderCache().get(textured ? "mle/model_pbr/cartoon_texture.frag" : "mle/model_pbr/cartoon_color.frag");
                break;
            case ModelTestShaderMode::WIREFRAME:
                mode_name = "wireframe";
                pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get(textured ? "mle/model_pbr/wire_texture.frag" : "mle/model_pbr/wire_color.frag");
                pipeline_ci.polygon_mode = vk::PolygonMode::eLine;
                {
                    std::array dynamic_states = {vk::DynamicState::eLineWidth};
                    pipeline_ci.dynamic_states = dynamic_states;
                    std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::COLOR)};
                    pipeline_ci.color_attachment_formats = color_attachment_formats;
                    auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
                    pipeline_ci.blend_attachments = blend_attachments;
                    pipeline_ci.topology = vk::PrimitiveTopology::eTriangleList;
                    pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
                    pipeline_ci.depth = true;
                    pipeline_ci.depth_write = true;
                    pipeline_ci.push_descriptor = 0;

                    const std::string name = fmt::format("model_test_{}_{}", mode_name, kind_idx);
                    pipelines[idx] = &Renderer::i().pipelineCache().setPipeline(name, pipeline_ci);
                    return pipelines[idx];
                }
                break;
            case ModelTestShaderMode::NORMALS:
                mode_name = "normals";
                pipeline_ci.fragment_shader =
                    &Renderer::i().shaderCache().get(textured ? "mle/model_pbr/normals_texture.frag" : "mle/model_pbr/normals_color.frag");
                break;
            case ModelTestShaderMode::ALBEDO:
                mode_name = "albedo";
                pipeline_ci.fragment_shader =
                    &Renderer::i().shaderCache().get(textured ? "mle/model_pbr/albedo_texture.frag" : "mle/model_pbr/albedo_color.frag");
                break;
            case ModelTestShaderMode::COUNT:
                MLE_UNREACHABLE;
        }

        std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::COLOR)};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
        pipeline_ci.blend_attachments = blend_attachments;
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleList;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        pipeline_ci.depth = true;
        pipeline_ci.depth_write = true;
        pipeline_ci.push_descriptor = 0;

        const std::string name = fmt::format("model_test_{}_{}", mode_name, kind_idx);
        pipelines[idx] = &Renderer::i().pipelineCache().setPipeline(name, pipeline_ci);
    }
    return pipelines[idx];
}

const Pipeline* getModelTestOutlinePipeline() {
    static const Pipeline* pipeline{};
    if (pipeline == nullptr) {
        Pipeline::CI pipeline_ci{};
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/fs_triangle.vert");
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/model_pbr/outline.frag");
        std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::COLOR)};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
        pipeline_ci.blend_attachments = blend_attachments;
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleList;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        pipeline_ci.depth = false;
        pipeline_ci.depth_write = false;
        pipeline_ci.push_descriptor = 0;

        pipeline = &Renderer::i().pipelineCache().setPipeline("model_test_cartoon_outline_screen", pipeline_ci);
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

mat4f makeViewProj(vec2u extent, f32 yaw, f32 pitch, f32 distance) {
    const f32 aspect = extent.y > 0 ? as<f32>(extent.x) / as<f32>(extent.y) : 1.0F;
    const vec3f target{0.0F, 0.15F, 0.0F};
    const f32 pitch_cos = std::cos(pitch);
    const vec3f orbit_dir{
        std::sin(yaw) * pitch_cos,
        std::sin(pitch),
        std::cos(yaw) * pitch_cos,
    };
    mat4f view = glm::lookAt(target + orbit_dir * distance, target, vec3f{0.0F, 1.0F, 0.0F});
    mat4f proj = glm::perspective(glm::radians(45.0F), aspect, 0.01F, 1000.0F);
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

entt::id_type makeAssetId(const std::string& name) {
    return entt::hashed_string::value(name.c_str());
}

int getDefaultSceneIndex(const tinygltf::Model& model) {
    if (model.scenes.empty()) {
        return -1;
    }
    if (model.defaultScene >= 0 && model.defaultScene < as<int>(model.scenes.size())) {
        return model.defaultScene;
    }
    return 0;
}

std::vector<ModelTestLayer::ModelOption> discoverModelOptions(const std::vector<std::string>& model_files) {
    std::vector<ModelTestLayer::ModelOption> options;

    for (const auto& model_file : model_files) {
        GLTF gltf;
        const Path model_path = Path{ResPath::RES} / ResPath::MODELS / model_file;
        if (gltf.load(model_path) != Result::OK) {
            MLE_W("ModelTestLayer failed to inspect model '{}'", model_path.generic_string());
            continue;
        }

        const auto& model = gltf.model();
        const int scene_index = getDefaultSceneIndex(model);
        if (scene_index < 0) {
            MLE_W("ModelTestLayer model '{}' has no scenes", model_file);
            continue;
        }

        std::unordered_set<std::string> names;
        const auto& scene = model.scenes[as<usize>(scene_index)];
        for (int node_index : scene.nodes) {
            if (node_index < 0 || node_index >= as<int>(model.nodes.size())) {
                MLE_W("ModelTestLayer model '{}' has invalid scene root node {}", model_file, node_index);
                continue;
            }

            const auto& node = model.nodes[as<usize>(node_index)];
            if (node.name.empty()) {
                MLE_W("ModelTestLayer skipping unnamed scene root in '{}'", model_file);
                continue;
            }
            if (!names.insert(node.name).second) {
                MLE_W("ModelTestLayer skipping duplicate scene root name '{}' in '{}'", node.name, model_file);
                continue;
            }

            options.push_back(ModelTestLayer::ModelOption{
                .key = fmt::format("{}#{}", model_file, node.name),
                .file = model_file,
                .root_node = as<usize>(node_index),
            });
        }
    }

    std::ranges::sort(options, [](const auto& lhs, const auto& rhs) { return lhs.key < rhs.key; });
    return options;
}

std::string makeAnimationDisplayName(const std::string& animation_file, AnimationClipRef clip) {
    MLE_ASSERT_LOG(clip != nullptr && !clip->getName().empty(), "Animation display names require a valid named animation clip.");
    return animation_file + "." + clip->getName();
}

bool animationTargetsModel(AnimationClipRef animation, ModelRef model) {
    if (animation == nullptr || model == nullptr) {
        return false;
    }

    const auto& binding = Renderer::i().animationCache().getBinding(model, animation);
    return std::ranges::any_of(binding.channel_to_node_map, [](usize nid) { return nid != max<usize>(); });
}

bool nodeSubtreeHasSkinnedMeshes(const tinygltf::Model& model, usize node_index) {
    if (node_index >= model.nodes.size()) {
        return false;
    }

    const auto& node = model.nodes[node_index];
    if (node.skin >= 0) {
        return true;
    }

    if (node.mesh >= 0) {
        MLE_ASSERT_LOG(node.mesh < as<int>(model.meshes.size()), "Invalid mesh index in node");
        const auto& mesh = model.meshes[as<usize>(node.mesh)];
        for (const auto& primitive : mesh.primitives) {
            if (primitive.attributes.contains("JOINTS_0") || primitive.attributes.contains("WEIGHTS_0")) {
                return true;
            }
        }
    }

    for (int child : node.children) {
        if (child >= 0 && child < as<int>(model.nodes.size()) && nodeSubtreeHasSkinnedMeshes(model, as<usize>(child))) {
            return true;
        }
    }

    return false;
}

bool nodeSubtreeHasMeshes(const tinygltf::Model& model, usize node_index) {
    if (node_index >= model.nodes.size()) {
        return false;
    }

    const auto& node = model.nodes[node_index];
    if (node.mesh >= 0) {
        return true;
    }

    for (int child : node.children) {
        if (child >= 0 && child < as<int>(model.nodes.size()) && nodeSubtreeHasMeshes(model, as<usize>(child))) {
            return true;
        }
    }

    return false;
}

std::vector<ModelTestLayer::ModelOption> discoverHeldItemOptions(const std::vector<ModelTestLayer::ModelOption>& model_options) {
    std::vector<ModelTestLayer::ModelOption> options;

    for (const auto& model_option : model_options) {
        GLTF gltf;
        const Path model_path = Path{ResPath::RES} / ResPath::MODELS / model_option.file;
        if (gltf.load(model_path) != Result::OK) {
            MLE_W("ModelTestLayer failed to inspect held item '{}'", model_path.generic_string());
            continue;
        }

        if (nodeSubtreeHasMeshes(gltf.model(), model_option.root_node) && !nodeSubtreeHasSkinnedMeshes(gltf.model(), model_option.root_node)) {
            options.push_back(model_option);
        }
    }

    return options;
}

MaterialUniform makeMaterialUniform(const Mesh::PbrMaterial& material) {
    return MaterialUniform{
        .base_color_factor = material.base_color_factor,
        .emissive_factor = vec4f{material.emissive_factor, 0.0F},
        .pbr_factors = vec4f{material.metallic_factor, material.roughness_factor, material.normal_scale, material.occlusion_strength},
    };
}

vec3f makeSunDirection(f32 yaw, f32 pitch) {
    const f32 pitch_cos = std::cos(pitch);
    return glm::normalize(vec3f{
        std::sin(yaw) * pitch_cos,
        -std::sin(pitch),
        std::cos(yaw) * pitch_cos,
    });
}

mat4f removeMatrixScale(const mat4f& transform) {
    mat4f out = transform;
    for (usize col = 0; col < 3; ++col) {
        const vec3f axis{out[col]};
        const f32 len = glm::length(axis);
        if (len > 1.0e-6F) {
            out[col] = vec4f{axis / len, 0.0F};
        }
    }
    return out;
}
}  // namespace

void ModelTestLayer::init() {
    MLE_I("ModelTestLayer::init()");

    refreshAssets();

    for (usize mode_idx = 0; mode_idx < as<usize>(ModelTestShaderMode::COUNT); ++mode_idx) {
        const auto mode = static_cast<ModelTestShaderMode>(mode_idx);
        getModelTestPipeline(Mesh::VertexKind::PBR_COLOR, mode);
        getModelTestPipeline(Mesh::VertexKind::PBR_COLOR_SKINNED, mode);
        getModelTestPipeline(Mesh::VertexKind::PBR_TEXTURE, mode);
        getModelTestPipeline(Mesh::VertexKind::PBR_TEXTURE_SKINNED, mode);
    }
    getModelTestOutlinePipeline();
    Client::i().getGameLayerTable()["model_test_set_camera_yaw"] = [this](f32 value) { setCameraYaw01(value); };
    Client::i().getGameLayerTable()["model_test_set_camera_pitch"] = [this](f32 value) { setCameraPitch01(value); };
    Client::i().getGameLayerTable()["model_test_set_camera_distance"] = [this](f32 value) { setCameraDistance01(value); };
    Client::i().getGameLayerTable()["model_test_set_sun_yaw"] = [this](f32 value) { setSunYaw01(value); };
    Client::i().getGameLayerTable()["model_test_set_sun_pitch"] = [this](f32 value) { setSunPitch01(value); };
    Client::i().getGameLayerTable()["model_test_set_sun_intensity"] = [this](f32 value) { setSunIntensity01(value); };
    Client::i().getGameLayerTable()["model_test_set_ambient"] = [this](f32 value) { setAmbient01(value); };
    Client::i().getGameLayerTable()["model_test_set_outline_width"] = [this](f32 value) { setOutlineWidth01(value); };
    Client::i().getGameLayerTable()["model_test_set_wireframe_width"] = [this](f32 value) { setWireframeWidth01(value); };
    Client::i().getGameLayerTable()["model_test_set_held_item_scale"] = [this](f32 value) { setHeldItemScale01(value); };
    Client::i().getGameLayerTable()["model_test_set_shader_mode"] = [this](const std::string& name) { setShaderMode(name); };
    Client::i().getGameLayerTable()["model_test_set_model"] = [this](const std::string& name) { setModel(name); };
    Client::i().getGameLayerTable()["model_test_set_held_item"] = [this](const std::string& name) { setHeldItem(name); };
    Client::i().getGameLayerTable()["model_test_set_animation"] = [this](const std::string& name) { setAnimation(name); };
    Client::i().getGameLayerTable()["model_test_clear_animation"] = [this]() { clearAnimation(); };
    Client::i().getGameLayerTable()["model_test_refresh_assets"] = [this]() { return refreshAssetsForLua(); };
    Client::i().getGameLayerTable()["model_test_held_item_names"] = [this]() { return makeHeldItemNamesTable(); };
    Client::i().getGameLayerTable()["model_test_model_names"] = makeModelNamesTable();
    Client::i().getGameLayerTable()["model_test_animation_names"] = makeAnimationNamesTable();
    Client::i().getGameLayerTable()["model_test_shader_mode_names"] = makeShaderModeNamesTable();
    ui_.setRoot("i/ui/ModelTestLayer");

    MLE_I("ModelTestLayer assets loaded: {} models, {} animations", model_options_.size(), animation_names_.size());
};

void ModelTestLayer::update() {
    animation_time_ += 1.0F / 60.0F;
    ui_.update();
};

ImageRef ModelTestLayer::render() {
    auto* image = render_target_.getImage(Color::WHITE);

    renderModel(image);

    if (auto* ui_image = ui_.render(); ui_image != nullptr) {
        image->blend(Renderer::i().frameRenderer().cmd(), *ui_image);
    }

    return image;
};

void ModelTestLayer::shutdown() {
    ui_.shutdown();
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

    auto& renderer = Renderer::i();
    const auto& meshes = model_->getMeshes();
    const auto& model_skins = model_->getSkins();

    node_globals_.resize(model_->getNodeCount());

    if (current_animation_ != nullptr) {
        const auto& binding = renderer.animationCache().getBinding(model_, current_animation_);
        current_animation_->evaluate(*model_, binding, animation_time_, node_globals_);
    } else {
        model_->evaluateBase(node_globals_);
    }

    skin_mats_.clear();
    for (usize skin_idx = 0; skin_idx < model_skins.size(); ++skin_idx) {
        const auto& skin_binding = model_skins[skin_idx];
        if (skin_binding.jointCount() == 0) {
            continue;
        }
        auto& mats = skin_mats_[as<int>(skin_idx)];
        mats.resize(skin_binding.jointCount());
        skin_binding.buildSkinMatrices(node_globals_, mats);
    }

    auto& frame_renderer = renderer.frameRenderer();

    RenderingThread thread;
    thread.init();

    std::unordered_map<int, vk::DescriptorBufferInfo> skin_mats_dis;
    for (const auto& [skin_index, skin_mats] : skin_mats_) {
        if (skin_mats.empty()) {
            continue;
        }

        BufferSlice skin_mats_slice = frame_renderer.getHostVisibleBuffer(sizeof(mat4f) * skin_mats.size(), vk::BufferUsageFlagBits::eStorageBuffer);
        skin_mats_slice.buffer->write(skin_mats.data(), skin_mats_slice.size, skin_mats_slice.offset);
        skin_mats_dis.emplace(skin_index, skin_mats_slice.buffer->makeDescriptorInfo(thread.cmd(), skin_mats_slice.size, skin_mats_slice.offset));
    }

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

    struct PushConstants {
        mat4f model;
        mat4f view_proj;
    } pc{};

    const mat4f preview_model = makeModelMatrix(meshes);
    pc.view_proj = makeViewProj(target->getExtent(), camera_yaw_, camera_pitch_, camera_distance_);

    LightingUniform lighting_uniform{};
    lighting_uniform.sun_direction_intensity = vec4f{makeSunDirection(sun_yaw_, sun_pitch_), sun_intensity_};
    lighting_uniform.sun_color_ambient = vec4f{1.0F, 0.955F, 0.88F, ambient_};
    BufferSlice lighting_slice = frame_renderer.getHostVisibleBuffer(sizeof(LightingUniform), vk::BufferUsageFlagBits::eUniformBuffer);
    lighting_slice.buffer->write(&lighting_uniform, lighting_slice.size, lighting_slice.offset);
    vk::DescriptorBufferInfo lighting_di = lighting_slice.buffer->makeDescriptorInfo(thread.cmd(), lighting_slice.size, lighting_slice.offset);

    auto draw_model_meshes = [&](const std::vector<Model::NodeMesh>& draw_meshes, const mat4f& model_matrix, bool allow_skinned,
                                 const std::vector<mat4f>* draw_node_globals) {
        for (const auto& node_mesh : draw_meshes) {
            const Mesh& mesh = node_mesh.mesh;
            if (mesh.getIndexCount() == 0 || (mesh.isSkinned() && !allow_skinned)) {
                continue;
            }

            pc.model = model_matrix;
            if (!mesh.isSkinned() && draw_node_globals != nullptr && node_mesh.node_index < draw_node_globals->size()) {
                pc.model *= draw_node_globals->at(node_mesh.node_index);
            }

            const auto skin_mats_di_it = skin_mats_dis.find(node_mesh.skin_index);
            if (mesh.isSkinned() && skin_mats_di_it == skin_mats_dis.end()) {
                continue;
            }
            const vk::DescriptorBufferInfo* skin_mats_di = mesh.isSkinned() ? &skin_mats_di_it->second : nullptr;

            const Pipeline* pipeline = getModelTestPipeline(mesh.getVertexKind(), shader_mode_);
            thread.setPipeline(pipeline);
            if (shader_mode_ == ModelTestShaderMode::WIREFRAME) {
                thread.setLineWidth(wireframe_width_);
            }

            const auto material_uniform = makeMaterialUniform(mesh.getMaterial());
            BufferSlice material_slice = frame_renderer.getHostVisibleBuffer(sizeof(MaterialUniform), vk::BufferUsageFlagBits::eUniformBuffer);
            material_slice.buffer->write(&material_uniform, material_slice.size, material_slice.offset);
            vk::DescriptorBufferInfo material_di = material_slice.buffer->makeDescriptorInfo(thread.cmd(), material_slice.size, material_slice.offset);

            const auto& material = mesh.getMaterial();
            if (mesh.isTextured()) {
                vk::DescriptorImageInfo base_color_di = material.base_color_texture->getDescriptorInfo();
                vk::DescriptorImageInfo metallic_roughness_di = material.metallic_roughness_texture->getDescriptorInfo();
                vk::DescriptorImageInfo normal_di = material.normal_texture->getDescriptorInfo();
                vk::DescriptorImageInfo occlusion_di = material.occlusion_texture->getDescriptorInfo();
                vk::DescriptorImageInfo emissive_di = material.emissive_texture->getDescriptorInfo();

                switch (shader_mode_) {
                    case ModelTestShaderMode::PBR:
                    case ModelTestShaderMode::CARTOON:
                        if (mesh.isSkinned()) {
                            auto push_writes = pipeline->makeWrites(0, nullptr, &material_di, &lighting_di, &base_color_di, &metallic_roughness_di,
                                                                    &normal_di, &occlusion_di, &emissive_di, skin_mats_di);
                            thread.pushDescriptor(0, push_writes);
                        } else {
                            auto push_writes = pipeline->makeWrites(0, nullptr, &material_di, &lighting_di, &base_color_di, &metallic_roughness_di,
                                                                    &normal_di, &occlusion_di, &emissive_di);
                            thread.pushDescriptor(0, push_writes);
                        }
                        break;
                    case ModelTestShaderMode::WIREFRAME:
                    case ModelTestShaderMode::NORMALS:
                    case ModelTestShaderMode::ALBEDO:
                        if (mesh.isSkinned()) {
                            auto push_writes = pipeline->makeWrites(0, nullptr, &material_di, &base_color_di, skin_mats_di);
                            thread.pushDescriptor(0, push_writes);
                        } else {
                            auto push_writes = pipeline->makeWrites(0, nullptr, &material_di, &base_color_di);
                            thread.pushDescriptor(0, push_writes);
                        }
                        break;
                    case ModelTestShaderMode::COUNT:
                        MLE_UNREACHABLE;
                }
            } else if (mesh.isSkinned()) {
                if (shader_mode_ == ModelTestShaderMode::PBR || shader_mode_ == ModelTestShaderMode::CARTOON) {
                    auto push_writes = pipeline->makeWrites(0, nullptr, &material_di, &lighting_di, skin_mats_di);
                    thread.pushDescriptor(0, push_writes);
                } else {
                    auto push_writes = pipeline->makeWrites(0, nullptr, &material_di, skin_mats_di);
                    thread.pushDescriptor(0, push_writes);
                }
            } else {
                if (shader_mode_ == ModelTestShaderMode::PBR || shader_mode_ == ModelTestShaderMode::CARTOON) {
                    auto push_writes = pipeline->makeWrites(0, nullptr, &material_di, &lighting_di);
                    thread.pushDescriptor(0, push_writes);
                } else {
                    auto push_writes = pipeline->makeWrites(0, nullptr, &material_di);
                    thread.pushDescriptor(0, push_writes);
                }
            }

            thread.pushConstants(&pc);
            thread.bindVertexBuffer(mesh.getVertexBuffer());
            thread.bindIndexBuffer(mesh.getIndexBuffer());
            thread.drawIndexed(as<u32>(mesh.getIndexCount()), 1);
        }
    };

    draw_model_meshes(meshes, preview_model, true, &node_globals_);

    if (held_item_model_ != nullptr && !held_item_model_->getMeshes().empty()) {
        usize attachment_node = model_->getNodeIdxByName("mixamorig:RightHand");
        if (attachment_node == max<usize>()) {
            attachment_node = model_->getNodeIdxByName("RightHand");
        }

        if (attachment_node != max<usize>() && attachment_node < node_globals_.size()) {
            std::vector<mat4f> held_item_node_globals;
            held_item_model_->evaluateBase(held_item_node_globals);

            const mat4f attachment_model = removeMatrixScale(node_globals_[attachment_node]);
            const mat4f held_item_scale = glm::scale(mat4f{1.0F}, vec3f{held_item_scale_});
            const mat4f held_item_model = preview_model * attachment_model * held_item_scale;
            draw_model_meshes(held_item_model_->getMeshes(), held_item_model, false, &held_item_node_globals);
        } else {
            const std::string warning_key = current_model_name_ + "|" + current_held_item_name_;
            if (held_item_attachment_warning_key_ != warning_key) {
                MLE_W("ModelTestLayer model '{}' has no mixamorig:RightHand or RightHand node for held item '{}'", current_model_name_,
                      current_held_item_name_);
                held_item_attachment_warning_key_ = warning_key;
            }
        }
    }

    if (shader_mode_ == ModelTestShaderMode::CARTOON) {
        thread.endRendering();

        depth_attachment.image->transitionState(thread.cmd(), Image::State::FS_READ);

        AttachmentInfo outline_color_attachment{};
        outline_color_attachment.image = target;
        outline_color_attachment.load_op = vk::AttachmentLoadOp::eLoad;
        thread.setColorAttachment(outline_color_attachment, 0);
        thread.setDepthAttachment({});
        thread.beginRendering();
        thread.setViewportAndScissor(Rectf{0.0F, 0.0F, as<f32>(target->getExtent().x), as<f32>(target->getExtent().y)});

        const Pipeline* outline_pipeline = getModelTestOutlinePipeline();
        thread.setPipeline(outline_pipeline);

        auto depth_di = depth_attachment.image->getDescriptorInfo();
        auto outline_writes = outline_pipeline->makeWrites(0, nullptr, &depth_di);
        thread.pushDescriptor(0, outline_writes);

        struct OutlinePushConstants {
            vec2f inv_extent;
            f32 depth_threshold;
            f32 alpha;
            f32 outline_width_px;
        } outline_pc{};

        outline_pc.inv_extent = 1.0F / vec2f{target->getExtent()};
        outline_pc.depth_threshold = 0.00075F;
        outline_pc.alpha = 0.95F;
        outline_pc.outline_width_px = outline_width_px_;

        thread.pushConstants(&outline_pc);
        thread.draw(3, 1);
    }

    thread.endRendering();

    thread.executeCommands();
}

void ModelTestLayer::refreshAssets() {
    model_files_ = discoverAssets(ResPath::MODELS);
    animation_files_ = discoverAssets(ResPath::ANIMATIONS);
    model_options_ = discoverModelOptions(model_files_);
    held_item_options_ = discoverHeldItemOptions(model_options_);

    model_ids_.clear();
    model_ids_.reserve(model_options_.size());
    for (const auto& model_option : model_options_) {
        model_ids_.push_back(makeAssetId(model_option.key));
    }

    animation_options_.clear();
    animation_names_.clear();

    auto& renderer = Renderer::i();
    for (const auto& animation_file : animation_files_) {
        GLTF animation_gltf;
        const Path animation_path = Path{ResPath::RES} / ResPath::ANIMATIONS / animation_file;
        if (animation_gltf.load(animation_path) != Result::OK) {
            MLE_W("ModelTestLayer failed to load animations '{}'", animation_path.generic_string());
            continue;
        }

        auto refs = renderer.animationCache().addAnimations(animation_file, animation_gltf);
        for (AnimationClipRef ref : refs) {
            const std::string label = makeAnimationDisplayName(animation_file, ref);
            animation_options_.push_back(AnimationOption{
                .label = label,
                .id = AnimationCache::makeAnimationId(animation_file, ref->getName()),
            });
            animation_names_.push_back(label);
        }
    }

    if (current_model_name_.empty() || std::ranges::find(model_options_, current_model_name_, &ModelOption::key) == model_options_.end()) {
        model_ = nullptr;
        skin_mats_.clear();
        current_model_name_.clear();
        if (!model_options_.empty()) {
            setModel(model_options_.front().key);
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

    if (current_held_item_name_.empty() || current_held_item_name_ == current_model_name_ ||
        std::ranges::find(held_item_options_, current_held_item_name_, &ModelOption::key) == held_item_options_.end()) {
        held_item_model_ = nullptr;
        current_held_item_name_.clear();
        held_item_attachment_warning_key_.clear();
    } else {
        setHeldItem(current_held_item_name_);
    }
}

sol::table ModelTestLayer::refreshAssetsForLua() {
    refreshAssets();

    auto ret = Client::i().lua().createTable();
    ret["models"] = makeModelNamesTable();
    ret["animations"] = makeAnimationNamesTable();
    ret["held_items"] = makeHeldItemNamesTable();
    return ret;
}

sol::table ModelTestLayer::makeModelNamesTable() const {
    auto table = Client::i().lua().createTable();
    for (usize i = 0; i < model_options_.size(); ++i) {
        table[i + 1] = model_options_[i].key;
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

sol::table ModelTestLayer::makeHeldItemNamesTable() const {
    auto table = Client::i().lua().createTable();
    table[1] = std::string{"None"};

    usize out_idx = 2;
    for (const auto& option : held_item_options_) {
        if (option.key == current_model_name_) {
            continue;
        }

        table[out_idx] = option.key;
        ++out_idx;
    }
    return table;
}

sol::table ModelTestLayer::makeShaderModeNamesTable() const {
    auto table = Client::i().lua().createTable();
    for (usize i = 0; i < SHADER_MODE_NAMES.size(); ++i) {
        table[i + 1] = std::string{SHADER_MODE_NAMES[i]};
    }
    return table;
}

bool ModelTestLayer::setModel(const std::string& name) {
    auto model_it = std::ranges::find(model_options_, name, &ModelOption::key);
    if (model_it == model_options_.end()) {
        MLE_W("ModelTestLayer model '{}' was not found", name);
        return false;
    }

    const usize model_idx = static_cast<usize>(std::distance(model_options_.begin(), model_it));
    const entt::id_type model_id = model_ids_.at(model_idx);

    GLTF model_gltf;
    const Path model_path = Path{ResPath::RES} / ResPath::MODELS / model_it->file;
    if (model_gltf.load(model_path) != Result::OK) {
        MLE_W("ModelTestLayer failed to load GLTF '{}'", model_path.generic_string());
        return false;
    }

    auto& model_cache = Renderer::i().modelCache();
    ModelRef model = model_cache.get(model_id);
    if (model == nullptr) {
        model = model_cache.add(model_id, model_gltf, model_it->root_node);
    }
    if (model == nullptr) {
        return false;
    }

    model_ = model;
    current_model_name_ = name;
    node_globals_.clear();
    skin_mats_.clear();
    animation_time_ = 0.0F;

    if (current_animation_ != nullptr && !animationTargetsModel(current_animation_, model_)) {
        MLE_W("ModelTestLayer animation '{}' has no channels matching model '{}'; base pose will be used", current_animation_name_, current_model_name_);
    }

    if (current_held_item_name_ == current_model_name_) {
        held_item_model_ = nullptr;
        current_held_item_name_.clear();
    }
    held_item_attachment_warning_key_.clear();

    return true;
}

bool ModelTestLayer::setHeldItem(const std::string& name) {
    if (name.empty() || name == "None") {
        held_item_model_ = nullptr;
        current_held_item_name_.clear();
        held_item_attachment_warning_key_.clear();
        return true;
    }

    auto item_it = std::ranges::find(held_item_options_, name, &ModelOption::key);
    if (item_it == held_item_options_.end()) {
        MLE_W("ModelTestLayer held item '{}' was not found", name);
        return false;
    }
    if (item_it->key == current_model_name_) {
        MLE_W("ModelTestLayer cannot use current model '{}' as its own held item", name);
        return false;
    }

    GLTF model_gltf;
    const Path model_path = Path{ResPath::RES} / ResPath::MODELS / item_it->file;
    if (model_gltf.load(model_path) != Result::OK) {
        MLE_W("ModelTestLayer failed to load held item GLTF '{}'", model_path.generic_string());
        return false;
    }

    const entt::id_type model_id = makeAssetId(item_it->key);
    auto& model_cache = Renderer::i().modelCache();
    ModelRef held_item_model = model_cache.get(model_id);
    if (held_item_model == nullptr) {
        held_item_model = model_cache.add(model_id, model_gltf, item_it->root_node);
    }
    if (held_item_model == nullptr) {
        return false;
    }

    for (const auto& node_mesh : held_item_model->getMeshes()) {
        if (node_mesh.mesh.isSkinned()) {
            MLE_W("ModelTestLayer held item '{}' contains skinned meshes and will not be used", name);
            return false;
        }
    }

    held_item_model_ = held_item_model;
    current_held_item_name_ = name;
    held_item_attachment_warning_key_.clear();
    return true;
}

void ModelTestLayer::setShaderMode(const std::string& name) {
    for (usize i = 0; i < SHADER_MODE_NAMES.size(); ++i) {
        if (SHADER_MODE_NAMES[i] == name) {
            shader_mode_ = static_cast<ModelTestShaderMode>(i);
            return;
        }
    }

    MLE_W("ModelTestLayer shader mode '{}' was not found", name);
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

void ModelTestLayer::setCameraDistance01(f32 value) {
    constexpr f32 MIN_DISTANCE = 0.01F;
    constexpr f32 MAX_DISTANCE = 100.0F;
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    camera_distance_ = MIN_DISTANCE + ((MAX_DISTANCE - MIN_DISTANCE) * clamped);
}

void ModelTestLayer::setSunYaw01(f32 value) {
    constexpr f32 TWO_PI = glm::radians(360.0F);
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    sun_yaw_ = (clamped - 0.5F) * TWO_PI;
}

void ModelTestLayer::setSunPitch01(f32 value) {
    constexpr f32 MIN_PITCH = glm::radians(5.0F);
    constexpr f32 MAX_PITCH = glm::radians(85.0F);
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    sun_pitch_ = MIN_PITCH + ((MAX_PITCH - MIN_PITCH) * clamped);
}

void ModelTestLayer::setSunIntensity01(f32 value) {
    constexpr f32 MIN_INTENSITY = 0.0F;
    constexpr f32 MAX_INTENSITY = 8.0F;
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    sun_intensity_ = MIN_INTENSITY + ((MAX_INTENSITY - MIN_INTENSITY) * clamped);
}

void ModelTestLayer::setAmbient01(f32 value) {
    constexpr f32 MIN_AMBIENT = 0.0F;
    constexpr f32 MAX_AMBIENT = 0.45F;
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    ambient_ = MIN_AMBIENT + ((MAX_AMBIENT - MIN_AMBIENT) * clamped);
}

void ModelTestLayer::setOutlineWidth01(f32 value) {
    constexpr f32 MIN_WIDTH = 0.5F;
    constexpr f32 MAX_WIDTH = 8.0F;
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    outline_width_px_ = MIN_WIDTH + ((MAX_WIDTH - MIN_WIDTH) * clamped);
}

void ModelTestLayer::setWireframeWidth01(f32 value) {
    constexpr f32 MIN_WIDTH = 1.0F;
    constexpr f32 MAX_WIDTH = 8.0F;
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    wireframe_width_ = MIN_WIDTH + ((MAX_WIDTH - MIN_WIDTH) * clamped);
}

void ModelTestLayer::setHeldItemScale01(f32 value) {
    constexpr f32 MIN_SCALE = 0.05F;
    constexpr f32 MAX_SCALE = 3.0F;
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    held_item_scale_ = MIN_SCALE + ((MAX_SCALE - MIN_SCALE) * clamped);
}

void ModelTestLayer::setAnimation(const std::string& name) {
    auto& animation_cache = Renderer::i().animationCache();
    for (const auto& option : animation_options_) {
        if (option.label == name) {
            current_animation_ = animation_cache.get(option.id);
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

void ModelTestLayer::clearAnimation() {
    current_animation_ = nullptr;
    current_animation_name_.clear();
    animation_time_ = 0.0F;
}
}  // namespace mle::user
