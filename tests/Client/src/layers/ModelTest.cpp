#include "ModelTest.h"

#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <string_view>
#include <unordered_map>

#include "Init.h"
#include "mle/client/Client.h"
#include "mle/core/Assert.h"
#include "mle/math/Types2D.h"
#include "mle/renderer/Animation.h"
#include "mle/renderer/Buffer.h"
#include "mle/renderer/GLTF.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/utils/ECS.h"
#include "mle/utils/File.h"

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
    vec4f camera_pos;
};

constexpr std::array<std::string_view, as<usize>(ModelTestShaderMode::COUNT)> SHADER_MODE_NAMES = {
    "PBR", "Cartoon", "Wireframe", "Normals", "Albedo",
};

template <usize Size>
auto makeNoBlendAttachments() {
    auto blend_attachments = Pipeline::makeDefaultBlendAttachments<Size>();
    for (auto& attachment : blend_attachments) {
        attachment.blendEnable = vk::False;
    }
    return blend_attachments;
}

const Pipeline* getModelTestGBufferPipeline(Primitive::VertexKind kind, bool wireframe) {
    static std::array<const Pipeline*, 4 * 2> pipelines{};
    const usize kind_idx = as<usize>(kind);
    const usize idx = (wireframe ? 4 : 0) + kind_idx;
    if (pipelines[idx] == nullptr) {
        MLE_I("ModelTest: creating G-buffer pipeline kind={} wireframe={}", kind_idx, wireframe);
        Pipeline::CI pipeline_ci{};
        switch (kind) {
            case Primitive::VertexKind::PBR_COLOR:
                pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/model_pbr/color.vert");
                break;
            case Primitive::VertexKind::PBR_COLOR_SKINNED:
                pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/model_pbr/color_skinned.vert");
                break;
            case Primitive::VertexKind::PBR_TEXTURE:
                pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/model_pbr/texture.vert");
                break;
            case Primitive::VertexKind::PBR_TEXTURE_SKINNED:
                pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/model_pbr/texture_skinned.vert");
                break;
        }

        const bool textured = kind == Primitive::VertexKind::PBR_TEXTURE || kind == Primitive::VertexKind::PBR_TEXTURE_SKINNED;
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get(textured ? "mle/model_pbr/gbuffer_texture.frag" : "mle/model_pbr/gbuffer_color.frag");
        const auto color_format = Renderer::i().vk().getVkImageFormat(ImageFormat::COLOR);
        const auto normal_format = Renderer::i().vk().getVkImageFormat(ImageFormat::NORMALS);
        const auto params_format = Renderer::i().vk().getVkImageFormat(ImageFormat::GBUF_PARAMS);
        std::array color_attachment_formats = {color_format, normal_format, params_format, color_format};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = makeNoBlendAttachments<4>();
        pipeline_ci.blend_attachments = blend_attachments;
        if (wireframe) {
            static std::array dynamic_states = {vk::DynamicState::eLineWidth};
            pipeline_ci.dynamic_states = dynamic_states;
            pipeline_ci.polygon_mode = vk::PolygonMode::eLine;
        }
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleList;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        pipeline_ci.depth = true;
        pipeline_ci.depth_write = true;
        pipeline_ci.push_descriptor = 0;

        const std::string name = fmt::format("model_test_gbuffer_{}_{}", wireframe ? "wire" : "fill", kind_idx);
        pipelines[idx] = &Renderer::i().pipelineCache().setPipeline(name, pipeline_ci);
        MLE_I("ModelTest: created G-buffer pipeline '{}'", name);
    }
    return pipelines[idx];
}

const Pipeline* getModelTestResolvePipeline(ModelTestShaderMode mode) {
    static std::array<const Pipeline*, as<usize>(ModelTestShaderMode::COUNT)> pipelines{};
    const usize idx = as<usize>(mode);
    if (pipelines[idx] == nullptr) {
        MLE_I("ModelTest: creating resolve pipeline mode={}", SHADER_MODE_NAMES.at(idx));
        Pipeline::CI pipeline_ci{};
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/fs_triangle.vert");

        std::string_view mode_name = "pbr";
        switch (mode) {
            case ModelTestShaderMode::PBR:
                mode_name = "pbr";
                pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/model_pbr/pbr_resolve.frag");
                break;
            case ModelTestShaderMode::CARTOON:
                mode_name = "cartoon";
                pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/model_pbr/cartoon_resolve.frag");
                break;
            case ModelTestShaderMode::WIREFRAME:
                mode_name = "wireframe";
                pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/model_pbr/wireframe_resolve.frag");
                break;
            case ModelTestShaderMode::NORMALS:
                mode_name = "normals";
                pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/model_pbr/normals_resolve.frag");
                break;
            case ModelTestShaderMode::ALBEDO:
                mode_name = "albedo";
                pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/model_pbr/albedo_resolve.frag");
                break;
            case ModelTestShaderMode::COUNT:
                MLE_UNREACHABLE;
        }

        std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::HDR_COLOR)};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
        blend_attachments[0].blendEnable = vk::False;
        pipeline_ci.blend_attachments = blend_attachments;
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleList;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        pipeline_ci.depth = false;
        pipeline_ci.depth_write = false;
        pipeline_ci.push_descriptor = 0;

        const std::string name = fmt::format("model_test_{}_resolve", mode_name);
        pipelines[idx] = &Renderer::i().pipelineCache().setPipeline(name, pipeline_ci);
        MLE_I("ModelTest: created resolve pipeline '{}'", name);
    }
    return pipelines[idx];
}

const Pipeline* getModelTestOutlinePipeline() {
    static const Pipeline* pipeline{};
    if (pipeline == nullptr) {
        MLE_I("ModelTest: creating cartoon outline pipeline");
        Pipeline::CI pipeline_ci{};
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/fs_triangle.vert");
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/model_pbr/outline.frag");
        std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::HDR_COLOR)};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
        pipeline_ci.blend_attachments = blend_attachments;
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleList;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        pipeline_ci.depth = false;
        pipeline_ci.depth_write = false;
        pipeline_ci.push_descriptor = 0;

        pipeline = &Renderer::i().pipelineCache().setPipeline("model_test_cartoon_outline_screen", pipeline_ci);
        MLE_I("ModelTest: created cartoon outline pipeline");
    }
    return pipeline;
}

const Pipeline* getModelTestTonemapPipeline() {
    static const Pipeline* pipeline{};
    if (pipeline == nullptr) {
        MLE_I("ModelTest: creating HDR tonemap pipeline");
        Pipeline::CI pipeline_ci{};
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/fs_triangle.vert");
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/model_pbr/tonemap.frag");
        std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::HDR_COLOR)};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
        blend_attachments[0].blendEnable = vk::False;
        pipeline_ci.blend_attachments = blend_attachments;
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleList;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        pipeline_ci.depth = false;
        pipeline_ci.depth_write = false;
        pipeline_ci.push_descriptor = 0;

        pipeline = &Renderer::i().pipelineCache().setPipeline("model_test_hdr_tonemap", pipeline_ci);
        MLE_I("ModelTest: created HDR tonemap pipeline");
    }
    return pipeline;
}

mat4f makeModelMatrix(const std::vector<Mesh::NodePrimitive>& meshes) {
    vec3f min_v{+FLT_MAX, +FLT_MAX, +FLT_MAX};
    vec3f max_v{-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (const auto& node_primitive : meshes) {
        const Primitive& primitive = node_primitive.primitive;
        if (primitive.getIndexCount() == 0) {
            continue;
        }

        min_v = glm::min(min_v, primitive.min());
        max_v = glm::max(max_v, primitive.max());
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

std::vector<ModelTestLayer::ModelOption> discoverModelOptions(const std::vector<std::string>& model_files) {
    std::vector<ModelTestLayer::ModelOption> options;

    for (const auto& model_file : model_files) {
        GLTF gltf;
        const Path model_path = Path{ResPath::RES} / ResPath::MODELS / model_file;
        if (gltf.load(model_path) != Result::OK) {
            MLE_W("ModelTestLayer failed to inspect model '{}'", model_path.generic_string());
            continue;
        }

        const auto mesh_nodes = gltf.getDefaultSceneMeshNodes();
        if (mesh_nodes.empty() && gltf.defaultSceneIndex() < 0) {
            MLE_W("ModelTestLayer model '{}' has no scenes", model_file);
            continue;
        }

        for (const auto& mesh_node : mesh_nodes) {
            options.push_back(ModelTestLayer::ModelOption{
                .key = fmt::format("{}#{}", model_file, mesh_node.name),
                .file = model_file,
                .root_node = mesh_node.node_index,
            });
        }
    }

    std::ranges::sort(options, [](const auto& lhs, const auto& rhs) { return lhs.key < rhs.key; });
    return options;
}

std::string makeAnimationDisplayName(const std::string& animation_file, AnimationClipRef clip) {
    MLE_ASSERT_LOG(clip != nullptr && !clip->getName().empty(), "Animation display names require a valid named animation clip.");
    return animation_file + "#" + clip->getName();
}

bool animationTargetsModel(AnimationClipRef animation, MeshRef model) {
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

MaterialUniform makeMaterialUniform(const Primitive::PbrMaterial& material) {
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
    MLE_I("ModelTest: assets refreshed. models={}, held_items={}, animations={}", model_options_.size(), held_item_options_.size(), animation_options_.size());

    for (bool wireframe : {false, true}) {
        MLE_I("ModelTest: warming G-buffer pipelines wireframe={}", wireframe);
        getModelTestGBufferPipeline(Primitive::VertexKind::PBR_COLOR, wireframe);
        getModelTestGBufferPipeline(Primitive::VertexKind::PBR_COLOR_SKINNED, wireframe);
        getModelTestGBufferPipeline(Primitive::VertexKind::PBR_TEXTURE, wireframe);
        getModelTestGBufferPipeline(Primitive::VertexKind::PBR_TEXTURE_SKINNED, wireframe);
    }
    for (usize mode_idx = 0; mode_idx < as<usize>(ModelTestShaderMode::COUNT); ++mode_idx) {
        MLE_I("ModelTest: warming resolve pipeline mode={}", SHADER_MODE_NAMES.at(mode_idx));
        getModelTestResolvePipeline(static_cast<ModelTestShaderMode>(mode_idx));
    }
    MLE_I("ModelTest: warming outline pipeline");
    getModelTestOutlinePipeline();
    MLE_I("ModelTest: warming HDR tonemap pipeline");
    getModelTestTonemapPipeline();
    MLE_I("ModelTest: pipeline warmup complete");
    Client::i().getGameLayerTable()["model_test_set_camera_yaw"] = [this](f32 value) { setCameraYaw01(value); };
    Client::i().getGameLayerTable()["model_test_set_camera_pitch"] = [this](f32 value) { setCameraPitch01(value); };
    Client::i().getGameLayerTable()["model_test_set_camera_distance"] = [this](f32 value) { setCameraDistance01(value); };
    Client::i().getGameLayerTable()["model_test_set_sun_yaw"] = [this](f32 value) { setSunYaw01(value); };
    Client::i().getGameLayerTable()["model_test_set_sun_pitch"] = [this](f32 value) { setSunPitch01(value); };
    Client::i().getGameLayerTable()["model_test_set_sun_intensity"] = [this](f32 value) { setSunIntensity01(value); };
    Client::i().getGameLayerTable()["model_test_set_ambient"] = [this](f32 value) { setAmbient01(value); };
    Client::i().getGameLayerTable()["model_test_set_outline_width"] = [this](f32 value) { setOutlineWidth01(value); };
    Client::i().getGameLayerTable()["model_test_set_outline_normal_threshold"] = [this](f32 value) { setOutlineNormalThreshold01(value); };
    Client::i().getGameLayerTable()["model_test_set_toon_band_softness"] = [this](f32 value) { setToonBandSoftness01(value); };
    Client::i().getGameLayerTable()["model_test_set_toon_shadow_level"] = [this](f32 value) { setToonShadowLevel01(value); };
    Client::i().getGameLayerTable()["model_test_set_toon_mid_level"] = [this](f32 value) { setToonMidLevel01(value); };
    Client::i().getGameLayerTable()["model_test_set_toon_highlight_level"] = [this](f32 value) { setToonHighlightLevel01(value); };
    Client::i().getGameLayerTable()["model_test_set_toon_spec_strength"] = [this](f32 value) { setToonSpecStrength01(value); };
    Client::i().getGameLayerTable()["model_test_set_toon_rim_strength"] = [this](f32 value) { setToonRimStrength01(value); };
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
    Client::i().getGameLayerTable()["return_to_init"] = []() { Client::i().pushGameLayer(std::make_unique<InitLayer>()); };
    Client::i().getGameLayerTable()["model_test_set_show_projection"] = [this](bool value) { show_projection_ = value; };
    Client::i().getGameLayerTable()["model_test_set_projection_epsilon"] = [this](f32 value) { projection_epsilon_ = value; };
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

GBuffer& ModelTestLayer::getGBuffer(vec2u size) {
    auto& frame_renderer = Renderer::i().frameRenderer();
    auto frame_idx = frame_renderer.getCurrentFrameId();
    auto& gbuffer = gbuffers_.at(frame_idx);

    auto ensure_image = [&](ImageHnd& image, Image::Format format, std::string_view label) {
        if (!image || image->getExtent() != size) {
            MLE_I("ModelTest: creating G-buffer image '{}' frame={} size={}x{} format={}", label, frame_idx, size.x, size.y, format);
            if (image) {
                frame_renderer.deleteAfterFrame(std::move(image));
            }
            Image::CI image_ci{};
            image_ci.extent = size;
            image_ci.format = format;
            image = Image::createHnd(image_ci);
            MLE_I("ModelTest: created G-buffer image '{}' image={}", label, fmt::ptr(image.get()));
        }
    };

    ensure_image(gbuffer.albedo, Image::Format::COLOR, "albedo");
    ensure_image(gbuffer.normal, Image::Format::NORMALS, "normal");
    ensure_image(gbuffer.params, Image::Format::GBUF_PARAMS, "params");
    ensure_image(gbuffer.emissive, Image::Format::COLOR, "emissive");
    ensure_image(gbuffer.depth, Image::Format::DEPTH, "depth");

    return gbuffer;
}

ImageRef ModelTestLayer::getHdrSceneImage(vec2u size) {
    auto& frame_renderer = Renderer::i().frameRenderer();
    auto frame_idx = frame_renderer.getCurrentFrameId();
    auto& image = hdr_scene_images_.at(frame_idx);
    if (!image || image->getExtent() != size) {
        MLE_I("ModelTest: creating HDR scene image frame={} size={}x{} format={}", frame_idx, size.x, size.y, Image::Format::HDR_COLOR);
        if (image) {
            frame_renderer.deleteAfterFrame(std::move(image));
        }
        Image::CI image_ci{};
        image_ci.extent = size;
        image_ci.format = Image::Format::HDR_COLOR;
        image = Image::createHnd(image_ci);
        MLE_I("ModelTest: created HDR scene image image={}", fmt::ptr(image.get()));
    }
    return image.get();
}

void ModelTestLayer::renderModel(ImageRef target) {
    if (!target || !model_ || model_->getPrimitives().empty()) {
        MLE_I("ModelTest: renderModel skipped target={} model={} mesh_count={}", fmt::ptr(target), fmt::ptr(model_), model_ ? model_->getPrimitives().size() : 0);
        return;
    }

    auto& renderer = Renderer::i();
    const auto& meshes = model_->getPrimitives();
    const auto& model_skins = model_->getSkins();
    MLE_D("ModelTest: renderModel begin target={} extent={}x{} mode={} meshes={} skins={}", fmt::ptr(target), target->getExtent().x, target->getExtent().y,
          SHADER_MODE_NAMES.at(as<usize>(shader_mode_)), meshes.size(), model_skins.size());

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

    GBuffer& gbuffer = getGBuffer(target->getExtent());
    ImageRef hdr_scene = getHdrSceneImage(target->getExtent());
    MLE_D("ModelTest: using G-buffer albedo={} normal={} params={} emissive={} depth={}", fmt::ptr(gbuffer.albedo.get()), fmt::ptr(gbuffer.normal.get()),
          fmt::ptr(gbuffer.params.get()), fmt::ptr(gbuffer.emissive.get()), fmt::ptr(gbuffer.depth.get()));
    MLE_D("ModelTest: using HDR scene image={}", fmt::ptr(hdr_scene));

    AttachmentInfo albedo_attachment{};
    albedo_attachment.image = gbuffer.albedo.get();
    albedo_attachment.load_op = vk::AttachmentLoadOp::eClear;
    albedo_attachment.clear_value.color = vk::ClearColorValue{std::array{1.0F, 1.0F, 1.0F, 1.0F}};

    AttachmentInfo normal_attachment{};
    normal_attachment.image = gbuffer.normal.get();
    normal_attachment.load_op = vk::AttachmentLoadOp::eClear;
    normal_attachment.clear_value.color = vk::ClearColorValue{std::array{0.5F, 0.5F, 0.0F, 0.0F}};

    AttachmentInfo params_attachment{};
    params_attachment.image = gbuffer.params.get();
    params_attachment.load_op = vk::AttachmentLoadOp::eClear;
    params_attachment.clear_value.color = vk::ClearColorValue{std::array{0.0F, 1.0F, 1.0F, 1.0F}};

    AttachmentInfo emissive_attachment{};
    emissive_attachment.image = gbuffer.emissive.get();
    emissive_attachment.load_op = vk::AttachmentLoadOp::eClear;
    emissive_attachment.clear_value.color = vk::ClearColorValue{std::array{0.0F, 0.0F, 0.0F, 0.0F}};

    std::array gbuffer_color_attachments = {albedo_attachment, normal_attachment, params_attachment, emissive_attachment};
    MLE_D("ModelTest: setting G-buffer attachments");
    thread.setColorAttachments(gbuffer_color_attachments);

    AttachmentInfo depth_attachment{};
    depth_attachment.image = gbuffer.depth.get();
    depth_attachment.load_op = vk::AttachmentLoadOp::eClear;
    depth_attachment.clear_value.depthStencil = vk::ClearDepthStencilValue{1.0F, 0};
    thread.setDepthAttachment(depth_attachment);

    MLE_D("ModelTest: begin G-buffer pass");
    thread.beginRendering();
    thread.setViewportAndScissor(Rectf{0.0F, 0.0F, as<f32>(target->getExtent().x), as<f32>(target->getExtent().y)});

    struct PushConstants {
        mat4f model;
        mat4f view_proj;
    } pc{};

    const mat4f preview_model = makeModelMatrix(meshes);
    pc.view_proj = makeViewProj(target->getExtent(), camera_yaw_, camera_pitch_, camera_distance_);
    const mat4f inv_view_proj = glm::inverse(pc.view_proj);

    const vec3f sun_dir = makeSunDirection(sun_yaw_, sun_pitch_);
    const f32 pitch_cos = std::cos(camera_pitch_);
    const vec3f orbit_dir{
        std::sin(camera_yaw_) * pitch_cos,
        std::sin(camera_pitch_),
        std::cos(camera_yaw_) * pitch_cos,
    };
    const vec3f camera_pos = vec3f{0.0F, 0.15F, 0.0F} + orbit_dir * camera_distance_;

    LightingUniform lighting_uniform{};
    lighting_uniform.sun_direction_intensity = vec4f{sun_dir, sun_intensity_};
    lighting_uniform.sun_color_ambient = vec4f{1.0F, 0.955F, 0.88F, ambient_};
    lighting_uniform.camera_pos = vec4f{camera_pos, 0.0F};
    BufferSlice lighting_slice = frame_renderer.getHostVisibleBuffer(sizeof(LightingUniform), vk::BufferUsageFlagBits::eUniformBuffer);
    lighting_slice.buffer->write(&lighting_uniform, lighting_slice.size, lighting_slice.offset);
    vk::DescriptorBufferInfo lighting_di = lighting_slice.buffer->makeDescriptorInfo(thread.cmd(), lighting_slice.size, lighting_slice.offset);

    auto draw_model_meshes = [&](const std::vector<Mesh::NodePrimitive>& draw_meshes, const mat4f& model_matrix, bool allow_skinned,
                                 const std::vector<mat4f>* draw_node_globals) {
        for (const auto& node_primitive : draw_meshes) {
            const Primitive& primitive = node_primitive.primitive;
            if (primitive.getIndexCount() == 0 || (primitive.isSkinned() && !allow_skinned)) {
                MLE_D("ModelTest: skipping mesh index_count={} skinned={} allow_skinned={}", primitive.getIndexCount(), primitive.isSkinned(), allow_skinned);
                continue;
            }

            pc.model = model_matrix;
            if (!primitive.isSkinned() && draw_node_globals != nullptr && node_primitive.node_index < draw_node_globals->size()) {
                pc.model *= draw_node_globals->at(node_primitive.node_index);
            }

            const auto skin_mats_di_it = skin_mats_dis.find(node_primitive.skin_index);
            if (primitive.isSkinned() && skin_mats_di_it == skin_mats_dis.end()) {
                MLE_W("ModelTest: skipping skinned mesh; no skin descriptor for skin_index={}", node_primitive.skin_index);
                continue;
            }
            const vk::DescriptorBufferInfo* skin_mats_di = primitive.isSkinned() ? &skin_mats_di_it->second : nullptr;

            MLE_D("ModelTest: drawing mesh vertex_kind={} textured={} skinned={} indices={} wireframe={}", as<usize>(primitive.getVertexKind()),
                  primitive.isTextured(), primitive.isSkinned(), primitive.getIndexCount(), shader_mode_ == ModelTestShaderMode::WIREFRAME);
            const Pipeline* pipeline = getModelTestGBufferPipeline(primitive.getVertexKind(), shader_mode_ == ModelTestShaderMode::WIREFRAME);
            thread.setPipeline(pipeline);
            if (shader_mode_ == ModelTestShaderMode::WIREFRAME) {
                thread.setLineWidth(wireframe_width_);
            }

            const auto material_uniform = makeMaterialUniform(primitive.getMaterial());
            BufferSlice material_slice = frame_renderer.getHostVisibleBuffer(sizeof(MaterialUniform), vk::BufferUsageFlagBits::eUniformBuffer);
            material_slice.buffer->write(&material_uniform, material_slice.size, material_slice.offset);
            vk::DescriptorBufferInfo material_di = material_slice.buffer->makeDescriptorInfo(thread.cmd(), material_slice.size, material_slice.offset);

            const auto& material = primitive.getMaterial();
            if (primitive.isTextured()) {
                vk::DescriptorImageInfo base_color_di = material.base_color_texture->getDescriptorInfo();
                vk::DescriptorImageInfo metallic_roughness_di = material.metallic_roughness_texture->getDescriptorInfo();
                vk::DescriptorImageInfo normal_di = material.normal_texture->getDescriptorInfo();
                vk::DescriptorImageInfo occlusion_di = material.occlusion_texture->getDescriptorInfo();
                vk::DescriptorImageInfo emissive_di = material.emissive_texture->getDescriptorInfo();

                if (primitive.isSkinned()) {
                    auto push_writes = pipeline->makeWrites(0, nullptr, &material_di, &base_color_di, &metallic_roughness_di, &normal_di, &occlusion_di,
                                                            &emissive_di, skin_mats_di);
                    thread.pushDescriptor(0, push_writes);
                } else {
                    auto push_writes =
                        pipeline->makeWrites(0, nullptr, &material_di, &base_color_di, &metallic_roughness_di, &normal_di, &occlusion_di, &emissive_di);
                    thread.pushDescriptor(0, push_writes);
                }
            } else if (primitive.isSkinned()) {
                auto push_writes = pipeline->makeWrites(0, nullptr, &material_di, skin_mats_di);
                thread.pushDescriptor(0, push_writes);
            } else {
                auto push_writes = pipeline->makeWrites(0, nullptr, &material_di);
                thread.pushDescriptor(0, push_writes);
            }

            thread.pushConstants(&pc);
            thread.bindVertexBuffer(primitive.getVertexBuffer());
            thread.bindIndexBuffer(primitive.getIndexBuffer());
            thread.drawIndexed(as<u32>(primitive.getIndexCount()), 1);
        }
    };

    draw_model_meshes(meshes, preview_model, true, &node_globals_);

    if (show_projection_ && model_ != nullptr) {
        Polygon2f proj_poly = model_->projectPolygon(Axis::Y);
        if (proj_poly.vertexCount() >= 3) {
            if (projection_epsilon_ > 0.0f) {
                proj_poly.simplify(projection_epsilon_);
            }

            const usize vertex_count = proj_poly.vertexCount();
            if (vertex_count >= 3) {
                std::vector<mat4f> base_node_globals(model_->getNodeCount());
                model_->evaluateBase(base_node_globals);
                f32 min_y = +FLT_MAX;
                for (const auto& np : meshes) {
                    mat4f xform = base_node_globals[np.node_index];
                    if (np.skin_index >= 0 && np.skin_index < as<int>(model_->getSkins().size())) {
                        const auto& skin = model_->getSkins()[np.skin_index];
                        if (skin.jointCount() > 0) {
                            const auto& joint = skin.getJoints()[0];
                            xform = base_node_globals[joint.node_index] * joint.inverse_bind;
                        }
                    }
                    for (const vec3f& local_pos : np.primitive.getCpuPositions()) {
                        const vec3f pos = vec3f(xform * vec4f(local_pos, 1.0f));
                        if (pos.y < min_y) {
                            min_y = pos.y;
                        }
                    }
                }
                if (min_y == +FLT_MAX) {
                    min_y = 0.0f;
                }

                const usize index_count = (vertex_count - 2) * 3;

                std::vector<Primitive::PbrColorVertex> vertices(vertex_count);
                for (usize i = 0; i < vertex_count; ++i) {
                    const vec2f& v = proj_poly.vertex(i);
                    vertices[i].pos = vec3f{v.x, min_y - 0.01f, v.y};
                    vertices[i].normal = vec3f{0.0f, 1.0f, 0.0f};
                    vertices[i].color = vec3f{0.0f, 0.8f, 1.0f};
                    vertices[i].mre = vec3f{0.0f, 1.0f, 0.0f};
                }

                std::vector<u32> indices(index_count);
                usize idx_ptr = 0;
                for (usize i = 1; i < vertex_count - 1; ++i) {
                    indices[idx_ptr++] = 0;
                    indices[idx_ptr++] = as<u32>(i);
                    indices[idx_ptr++] = as<u32>(i + 1);
                }

                BufferSlice vertex_slice = frame_renderer.getHostVisibleBuffer(sizeof(Primitive::PbrColorVertex) * vertex_count, vk::BufferUsageFlagBits::eVertexBuffer);
                vertex_slice.buffer->write(vertices.data(), vertex_slice.size, vertex_slice.offset);

                BufferSlice index_slice = frame_renderer.getHostVisibleBuffer(sizeof(u32) * index_count, vk::BufferUsageFlagBits::eIndexBuffer);
                index_slice.buffer->write(indices.data(), index_slice.size, index_slice.offset);

                Primitive::PbrMaterial proj_mat{};
                proj_mat.base_color_factor = vec4f{0.0F, 0.8F, 1.0F, 1.0F};
                const auto material_uniform = makeMaterialUniform(proj_mat);
                BufferSlice material_slice = frame_renderer.getHostVisibleBuffer(sizeof(MaterialUniform), vk::BufferUsageFlagBits::eUniformBuffer);
                material_slice.buffer->write(&material_uniform, material_slice.size, material_slice.offset);
                vk::DescriptorBufferInfo material_di = material_slice.buffer->makeDescriptorInfo(thread.cmd(), material_slice.size, material_slice.offset);

                pc.model = preview_model;
                const Pipeline* pipeline = getModelTestGBufferPipeline(Primitive::VertexKind::PBR_COLOR, false);
                thread.setPipeline(pipeline);

                auto push_writes = pipeline->makeWrites(0, nullptr, &material_di);
                thread.pushDescriptor(0, push_writes);

                thread.pushConstants(&pc);
                thread.bindVertexBuffer(vertex_slice.buffer, vertex_slice.offset);
                thread.bindIndexBuffer(index_slice.buffer, index_slice.offset);
                thread.drawIndexed(as<u32>(index_count), 1);
            }
        }
    }

    if (held_item_model_ != nullptr && !held_item_model_->getPrimitives().empty()) {
        usize attachment_node = model_->getNodeIdxByName("mixamorig:RightHand");
        if (attachment_node == max<usize>()) {
            attachment_node = model_->getNodeIdxByName("RightHand");
        }

        if (attachment_node != max<usize>() && attachment_node < node_globals_.size()) {
            std::vector<mat4f> held_item_node_globals(held_item_model_->getNodeCount());
            held_item_model_->evaluateBase(held_item_node_globals);

            const mat4f attachment_model = removeMatrixScale(node_globals_[attachment_node]);
            const mat4f held_item_scale = glm::scale(mat4f{1.0F}, vec3f{held_item_scale_});
            const mat4f held_item_model = preview_model * attachment_model * held_item_scale;
            draw_model_meshes(held_item_model_->getPrimitives(), held_item_model, false, &held_item_node_globals);
        } else {
            const std::string warning_key = current_model_name_ + "|" + current_held_item_name_;
            if (held_item_attachment_warning_key_ != warning_key) {
                MLE_W("ModelTestLayer model '{}' has no mixamorig:RightHand or RightHand node for held item '{}'", current_model_name_,
                      current_held_item_name_);
                held_item_attachment_warning_key_ = warning_key;
            }
        }
    }

    MLE_D("ModelTest: end G-buffer pass");
    thread.endRendering();

    MLE_D("ModelTest: transitioning G-buffer to FS_READ");
    gbuffer.albedo->transitionState(thread.cmd(), Image::State::FS_READ);
    gbuffer.normal->transitionState(thread.cmd(), Image::State::FS_READ);
    gbuffer.params->transitionState(thread.cmd(), Image::State::FS_READ);
    gbuffer.emissive->transitionState(thread.cmd(), Image::State::FS_READ);
    gbuffer.depth->transitionState(thread.cmd(), Image::State::FS_READ);

    AttachmentInfo resolve_color_attachment{};
    resolve_color_attachment.image = hdr_scene;
    resolve_color_attachment.load_op = vk::AttachmentLoadOp::eClear;
    resolve_color_attachment.clear_value.color = vk::ClearColorValue{std::array{0.0F, 0.0F, 0.0F, 0.0F}};
    std::array resolve_color_attachments = {resolve_color_attachment};
    thread.setColorAttachments(resolve_color_attachments);
    thread.setDepthAttachment({});
    MLE_D("ModelTest: begin resolve pass mode={}", SHADER_MODE_NAMES.at(as<usize>(shader_mode_)));
    thread.beginRendering();
    thread.setViewportAndScissor(Rectf{0.0F, 0.0F, as<f32>(target->getExtent().x), as<f32>(target->getExtent().y)});

    const Pipeline* resolve_pipeline = getModelTestResolvePipeline(shader_mode_);
    thread.setPipeline(resolve_pipeline);

    auto albedo_di = gbuffer.albedo->getDescriptorInfo();
    auto normal_di = gbuffer.normal->getDescriptorInfo();
    auto params_di = gbuffer.params->getDescriptorInfo();
    auto emissive_di = gbuffer.emissive->getDescriptorInfo();
    auto depth_di = gbuffer.depth->getDescriptorInfo();

    switch (shader_mode_) {
        case ModelTestShaderMode::PBR:
        case ModelTestShaderMode::CARTOON: {
            MLE_D("ModelTest: pushing lit resolve descriptors");
            auto resolve_writes = resolve_pipeline->makeWrites(0, nullptr, &albedo_di, &normal_di, &params_di, &emissive_di, &depth_di, &lighting_di);
            thread.pushDescriptor(0, resolve_writes);
            break;
        }
        case ModelTestShaderMode::WIREFRAME:
        case ModelTestShaderMode::NORMALS:
        case ModelTestShaderMode::ALBEDO: {
            MLE_D("ModelTest: pushing debug resolve descriptors");
            auto resolve_writes = resolve_pipeline->makeWrites(0, nullptr, &albedo_di, &normal_di, &depth_di);
            thread.pushDescriptor(0, resolve_writes);
            break;
        }
        case ModelTestShaderMode::COUNT:
            MLE_UNREACHABLE;
    }

    struct ResolvePushConstants {
        mat4f inv_view_proj;
        vec4f toon_levels;
        vec4f toon_params;
    } resolve_pc{
        .inv_view_proj = inv_view_proj,
        .toon_levels = vec4f{toon_shadow_level_, toon_mid_level_, toon_highlight_level_, toon_band_softness_},
        .toon_params = vec4f{toon_spec_strength_, toon_rim_strength_, 0.0F, 0.0F},
    };
    thread.pushConstants(&resolve_pc);
    MLE_D("ModelTest: draw resolve triangle");
    thread.draw(3, 1);

    if (shader_mode_ == ModelTestShaderMode::CARTOON) {
        MLE_D("ModelTest: begin cartoon outline overlay");
        const Pipeline* outline_pipeline = getModelTestOutlinePipeline();
        thread.setPipeline(outline_pipeline);

        auto outline_writes = outline_pipeline->makeWrites(0, nullptr, &depth_di, &normal_di);
        thread.pushDescriptor(0, outline_writes);

        struct OutlinePushConstants {
            vec2f inv_extent;
            f32 depth_threshold;
            f32 normal_threshold;
            f32 alpha;
            f32 outline_width_px;
        } outline_pc{};

        outline_pc.inv_extent = 1.0F / vec2f{target->getExtent()};
        outline_pc.depth_threshold = 0.00075F;
        outline_pc.normal_threshold = outline_normal_threshold_;
        outline_pc.alpha = 0.95F;
        outline_pc.outline_width_px = outline_width_px_;

        thread.pushConstants(&outline_pc);
        MLE_D("ModelTest: draw outline triangle");
        thread.draw(3, 1);
    }

    MLE_D("ModelTest: end resolve pass");
    thread.endRendering();

    MLE_D("ModelTest: transitioning HDR scene to FS_READ");
    hdr_scene->transitionState(thread.cmd(), Image::State::FS_READ);

    AttachmentInfo tonemap_color_attachment{};
    tonemap_color_attachment.image = target;
    tonemap_color_attachment.load_op = vk::AttachmentLoadOp::eLoad;
    std::array tonemap_color_attachments = {tonemap_color_attachment};
    thread.setColorAttachments(tonemap_color_attachments);
    thread.setDepthAttachment({});
    MLE_D("ModelTest: begin HDR tonemap pass mode={}", SHADER_MODE_NAMES.at(as<usize>(shader_mode_)));
    thread.beginRendering();
    thread.setViewportAndScissor(Rectf{0.0F, 0.0F, as<f32>(target->getExtent().x), as<f32>(target->getExtent().y)});

    const Pipeline* tonemap_pipeline = getModelTestTonemapPipeline();
    thread.setPipeline(tonemap_pipeline);
    auto hdr_scene_di = hdr_scene->getDescriptorInfo();
    auto tonemap_writes = tonemap_pipeline->makeWrites(0, nullptr, &hdr_scene_di);
    thread.pushDescriptor(0, tonemap_writes);

    struct TonemapPushConstants {
        vec4f params;
    } tonemap_pc{
        .params = vec4f{1.0F,
                        shader_mode_ == ModelTestShaderMode::WIREFRAME || shader_mode_ == ModelTestShaderMode::NORMALS ||
                                shader_mode_ == ModelTestShaderMode::ALBEDO
                            ? 1.0F
                            : 0.0F,
                        0.0F, 0.0F},
    };
    thread.pushConstants(&tonemap_pc);
    MLE_D("ModelTest: draw tonemap triangle");
    thread.draw(3, 1);
    MLE_D("ModelTest: end HDR tonemap pass");
    thread.endRendering();

    MLE_D("ModelTest: execute render commands");
    thread.executeCommands();
}

void ModelTestLayer::refreshAssets() {
    model_files_ = discoverAssets(ResPath::MODELS);
    animation_files_ = discoverAssets(ResPath::MODELS);

    auto& renderer = Renderer::i();
    for (const auto& file : model_files_) {
        renderer.addMeshPack(file);
    }

    model_options_ = discoverModelOptions(model_files_);
    held_item_options_ = discoverHeldItemOptions(model_options_);

    model_ids_.clear();
    model_ids_.reserve(model_options_.size());
    for (const auto& model_option : model_options_) {
        model_ids_.push_back(makeAssetId(model_option.key));
    }

    animation_options_.clear();
    animation_names_.clear();

    for (const auto& animation_file : animation_files_) {
        GLTF animation_gltf;
        const Path animation_path = Path{ResPath::RES} / ResPath::MODELS / animation_file;
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

    auto& model_cache = Renderer::i().meshCache();
    MeshRef model = model_cache.get(model_id);
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
    auto& model_cache = Renderer::i().meshCache();
    MeshRef held_item_model = model_cache.get(model_id);
    if (held_item_model == nullptr) {
        held_item_model = model_cache.add(model_id, model_gltf, item_it->root_node);
    }
    if (held_item_model == nullptr) {
        return false;
    }

    for (const auto& node_primitive : held_item_model->getPrimitives()) {
        if (node_primitive.primitive.isSkinned()) {
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

void ModelTestLayer::setOutlineNormalThreshold01(f32 value) {
    constexpr f32 MIN_THRESHOLD = 0.02F;
    constexpr f32 MAX_THRESHOLD = 0.50F;
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    outline_normal_threshold_ = MIN_THRESHOLD + ((MAX_THRESHOLD - MIN_THRESHOLD) * clamped);
}

void ModelTestLayer::setToonBandSoftness01(f32 value) {
    constexpr f32 MIN_SOFTNESS = 0.001F;
    constexpr f32 MAX_SOFTNESS = 0.08F;
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    toon_band_softness_ = MIN_SOFTNESS + ((MAX_SOFTNESS - MIN_SOFTNESS) * clamped);
}

void ModelTestLayer::setToonShadowLevel01(f32 value) {
    constexpr f32 MIN_LEVEL = 0.0F;
    constexpr f32 MAX_LEVEL = 1.5F;
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    toon_shadow_level_ = MIN_LEVEL + ((MAX_LEVEL - MIN_LEVEL) * clamped);
}

void ModelTestLayer::setToonMidLevel01(f32 value) {
    constexpr f32 MIN_LEVEL = 0.0F;
    constexpr f32 MAX_LEVEL = 1.5F;
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    toon_mid_level_ = MIN_LEVEL + ((MAX_LEVEL - MIN_LEVEL) * clamped);
}

void ModelTestLayer::setToonHighlightLevel01(f32 value) {
    constexpr f32 MIN_LEVEL = 0.0F;
    constexpr f32 MAX_LEVEL = 1.5F;
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    toon_highlight_level_ = MIN_LEVEL + ((MAX_LEVEL - MIN_LEVEL) * clamped);
}

void ModelTestLayer::setToonSpecStrength01(f32 value) {
    constexpr f32 MIN_STRENGTH = 0.0F;
    constexpr f32 MAX_STRENGTH = 2.0F;
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    toon_spec_strength_ = MIN_STRENGTH + ((MAX_STRENGTH - MIN_STRENGTH) * clamped);
}

void ModelTestLayer::setToonRimStrength01(f32 value) {
    constexpr f32 MIN_STRENGTH = 0.0F;
    constexpr f32 MAX_STRENGTH = 2.0F;
    const f32 clamped = std::clamp(value, 0.0F, 1.0F);
    toon_rim_strength_ = MIN_STRENGTH + ((MAX_STRENGTH - MIN_STRENGTH) * clamped);
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
