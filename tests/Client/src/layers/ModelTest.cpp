#include "ModelTest.h"

#include <spdlog/fmt/fmt.h>

#include <algorithm>
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
#include "mle/ui/Entt.h"
#include "mle/ui/components/Renderable.h"
#include "mle/ui/renderable/Text.h"
#include "mle/utils/ECS.h"
#include "mle/utils/File.h"
#include "mle/window/UserInputManager.h"

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
        const auto& descriptor = modelTestShaderModeDescriptor(mode);
        MLE_I("ModelTest: creating resolve pipeline mode={}", descriptor.display_name);
        Pipeline::CI pipeline_ci{};
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/fs_triangle.vert");
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get(std::string{descriptor.resolve_fragment_shader});

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

        pipelines[idx] = &Renderer::i().pipelineCache().setPipeline(std::string{descriptor.resolve_pipeline}, pipeline_ci);
        MLE_I("ModelTest: created resolve pipeline '{}'", descriptor.resolve_pipeline);
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

const Pipeline* getModelTestProjectionPipeline(bool blend) {
    static std::array<const Pipeline*, 2> pipelines{};
    const usize idx = blend ? 1 : 0;
    if (pipelines[idx] == nullptr) {
        const auto& descriptor = modelTestProjectionPipelineDescriptor();
        MLE_I("ModelTest: creating flat projection pipeline blend={}", blend);
        Pipeline::CI pipeline_ci{};
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get(std::string{descriptor.vertex_shader});
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get(std::string{descriptor.fragment_shader});
        std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::HDR_COLOR)};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
        blend_attachments[0].blendEnable = blend ? vk::True : vk::False;
        pipeline_ci.blend_attachments = blend_attachments;
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleFan;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        pipeline_ci.depth = false;
        pipeline_ci.depth_write = false;

        const std::string name = blend ? std::string{descriptor.pipeline} : std::string{descriptor.pipeline} + "_opaque";
        pipelines[idx] = &Renderer::i().pipelineCache().setPipeline(name, pipeline_ci);
        MLE_I("ModelTest: created flat projection pipeline '{}'", name);
    }
    return pipelines[idx];
}

const Pipeline* getModelTestTonemapPipeline(bool blend) {
    static std::array<const Pipeline*, 2> pipelines{};
    const usize idx = blend ? 1 : 0;
    if (pipelines[idx] == nullptr) {
        MLE_I("ModelTest: creating HDR tonemap pipeline blend={}", blend);
        Pipeline::CI pipeline_ci{};
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/fs_triangle.vert");
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/model_pbr/tonemap.frag");
        std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::HDR_COLOR)};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
        blend_attachments[0].blendEnable = blend ? vk::True : vk::False;
        pipeline_ci.blend_attachments = blend_attachments;
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleList;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        pipeline_ci.depth = false;
        pipeline_ci.depth_write = false;
        pipeline_ci.push_descriptor = 0;

        const std::string name = blend ? "model_test_hdr_tonemap_blend" : "model_test_hdr_tonemap";
        pipelines[idx] = &Renderer::i().pipelineCache().setPipeline(name, pipeline_ci);
        MLE_I("ModelTest: created HDR tonemap pipeline '{}'", name);
    }
    return pipelines[idx];
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

mat4f makeViewProj(vec2u extent, const ModelTestCameraState& camera) {
    const f32 aspect = extent.y > 0 ? as<f32>(extent.x) / as<f32>(extent.y) : 1.0F;
    const f32 pitch_cos = std::cos(camera.pitch);
    const vec3f orbit_dir{
        std::sin(camera.yaw) * pitch_cos,
        std::sin(camera.pitch),
        std::cos(camera.yaw) * pitch_cos,
    };
    mat4f view = glm::lookAt(camera.target + orbit_dir * camera.distance, camera.target, vec3f{0.0F, 1.0F, 0.0F});
    mat4f proj = glm::perspective(glm::radians(45.0F), aspect, 0.01F, 1000.0F);
    proj[1][1] *= -1.0F;
    return proj * view;
}

entt::id_type makeAssetId(const std::string& name) {
    return entt::hashed_string::value(name.c_str());
}

bool animationTargetsModel(AnimationClipRef animation, MeshRef model) {
    if (animation == nullptr || model == nullptr) {
        return false;
    }

    const auto& binding = Renderer::i().animationCache().getBinding(model, animation);
    return std::ranges::any_of(binding.channel_to_node_map, [](usize nid) { return nid != max<usize>(); });
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

bool hasFocusedTextInput(UI& ui) {
    auto view = ui.getRegistry().view<ui::comp::Renderable>();
    for (entt::entity entity : view) {
        const auto& renderable = view.get<ui::comp::Renderable>(entity);
        if (renderable.impl == nullptr || renderable.impl->getType() != ui::renderable::Text::type()) {
            continue;
        }
        const auto* text = as<const ui::renderable::Text*>(renderable.impl.get());
        if (text->input_tb != nullptr && text->input_tb->isFocused()) {
            return true;
        }
    }
    return false;
}

bool isUiCapturedAt(UI& ui, vec2f cursor_pos) {
    const entt::entity root = ui.getRoot();
    if (root == entt::null) {
        return false;
    }

    const entt::entity hit = ui.hoverSystem().hitTest(cursor_pos);
    if (hit != entt::null && hit != root) {
        return true;
    }

    constexpr std::array<std::string_view, 2> PANEL_PATH{"responsive_layout", "panel"};
    const auto panel = ui.getE(PANEL_PATH);
    return panel.has_value() && panel->getBoundsOnRoot().contains(cursor_pos);
}

}  // namespace

void ModelTestLayer::init() {
    MLE_I("ModelTestLayer::init()");

    initializeScene();

    for (bool wireframe : {false, true}) {
        MLE_I("ModelTest: warming G-buffer pipelines wireframe={}", wireframe);
        getModelTestGBufferPipeline(Primitive::VertexKind::PBR_COLOR, wireframe);
        getModelTestGBufferPipeline(Primitive::VertexKind::PBR_COLOR_SKINNED, wireframe);
        getModelTestGBufferPipeline(Primitive::VertexKind::PBR_TEXTURE, wireframe);
        getModelTestGBufferPipeline(Primitive::VertexKind::PBR_TEXTURE_SKINNED, wireframe);
    }
    for (const auto& descriptor : modelTestShaderModeDescriptors()) {
        MLE_I("ModelTest: warming resolve pipeline mode={}", descriptor.display_name);
        getModelTestResolvePipeline(descriptor.mode);
    }
    MLE_I("ModelTest: warming outline pipeline");
    getModelTestOutlinePipeline();
    MLE_I("ModelTest: warming flat projection pipeline");
    getModelTestProjectionPipeline(true);
    MLE_I("ModelTest: warming HDR tonemap pipeline");
    getModelTestTonemapPipeline(false);
    getModelTestTonemapPipeline(true);
    MLE_I("ModelTest: pipeline warmup complete");
    Client::i().getGameLayerTable()["model_test_reset_camera"] = [this]() { camera_.reset(); };
    Client::i().getGameLayerTable()["model_test_set_sun_yaw"] = [this](f32 value) { setSunYaw01(value); };
    Client::i().getGameLayerTable()["model_test_set_sun_pitch"] = [this](f32 value) { setSunPitch01(value); };
    Client::i().getGameLayerTable()["model_test_set_sun_intensity"] = [this](f32 value) { setSunIntensity01(value); };
    Client::i().getGameLayerTable()["model_test_set_ambient"] = [this](f32 value) { setAmbient01(value); };
    Client::i().getGameLayerTable()["model_test_set_held_item_translation"] = [this](f32 x, f32 y, f32 z) { setHeldItemTranslation(x, y, z); };
    Client::i().getGameLayerTable()["model_test_set_held_item_rotation"] = [this](f32 x, f32 y, f32 z) { setHeldItemRotation(x, y, z); };
    Client::i().getGameLayerTable()["model_test_set_held_item_scale"] = [this](f32 value) { setHeldItemScale(value); };
    bindModelTestShaderLua(Client::i().lua(), Client::i().getGameLayerTable(), renderer_state_);
    Client::i().getGameLayerTable()["model_test_submit_model"] = [this](const std::string& resource_id) { return submitModel(resource_id); };
    Client::i().getGameLayerTable()["model_test_submit_held_item"] = [this](const std::string& resource_id) { return submitHeldItem(resource_id); };
    Client::i().getGameLayerTable()["model_test_submit_animation"] = [this](const std::string& resource_id) { return submitAnimation(resource_id); };
    Client::i().getGameLayerTable()["model_test_submit_attachment"] = [this](const std::string& selector) { return submitAttachment(selector); };
    Client::i().getGameLayerTable()["model_test_complete_model"] = [this](const std::string& query) {
        return completeResourceForLua(ModelResourceKind::MODEL, query);
    };
    Client::i().getGameLayerTable()["model_test_complete_held_item"] = [this](const std::string& query) {
        return completeResourceForLua(ModelResourceKind::HELD_ITEM, query);
    };
    Client::i().getGameLayerTable()["model_test_complete_animation"] = [this](const std::string& query) {
        return completeResourceForLua(ModelResourceKind::ANIMATION, query);
    };
    Client::i().getGameLayerTable()["model_test_complete_attachment"] = [this](const std::string& query) {
        return completeResourceForLua(ModelResourceKind::ATTACHMENT_NODE, query);
    };
    Client::i().getGameLayerTable()["model_test_resource_status"] = [this]() {
        return scene_ != nullptr ? scene_->status() : std::string{"Resource state unavailable"};
    };
    Client::i().getGameLayerTable()["model_test_refresh_resource_paths"] = [this]() { return refreshResourcePaths(); };
    Client::i().getGameLayerTable()["model_test_clear_held_item"] = [this]() { clearHeldItem(); };
    Client::i().getGameLayerTable()["model_test_clear_animation"] = [this]() { clearAnimation(); };
    Client::i().getGameLayerTable()["return_to_init"] = []() { Client::i().pushGameLayer(std::make_unique<InitLayer>()); };
    Client::i().getGameLayerTable()["model_test_set_show_projection"] = [this](bool value) { show_projection_ = value; };
    Client::i().getGameLayerTable()["model_test_set_projection_epsilon"] = [this](f32 value) { projection_epsilon_ = value; };
    Client::i().getGameLayerTable()["model_test_set_clear_color"] = [this](Color color) { clear_color_srgb_ = color; };
    Client::i().getGameLayerTable()["model_test_set_projection_color"] = [this](Color color) { projection_color_srgb_ = color; };
    ui_.setRoot("i/ui/ModelTestLayer");

    MLE_I("ModelTestLayer resource state: {}", scene_ != nullptr ? scene_->status() : "unavailable");
};

void ModelTestLayer::update() {
    animation_time_ += 1.0F / 60.0F;
    ui_.update();

    constexpr std::array<std::string_view, 2> VIEWPORT_PATH{"responsive_layout", "viewport"};
    viewport_layout_ = {};
    if (const auto viewport = ui_.getE(VIEWPORT_PATH); viewport.has_value()) {
        viewport_layout_ = resolveModelTestViewportLayout(viewport->getBoundsOnRoot(), ui_.getRootMaxSize());
    }

    auto& input = UserInputManager::i();
    ModelTestCameraInput camera_input{
        .viewport_size_px = vec2f{viewport_layout_.render_extent},
        .left_pressed = input.isPressed(Key::MOUSE_LEFT),
        .left_down = input.isDown(Key::MOUSE_LEFT),
        .middle_pressed = input.isPressed(Key::MOUSE_MIDDLE),
        .middle_down = input.isDown(Key::MOUSE_MIDDLE),
    };
    if (const auto cursor_pos = input.getCursorPos(); cursor_pos.has_value()) {
        camera_input.cursor_inside_viewport = viewport_layout_.containsCursor(*cursor_pos);
        camera_input.ui_captured = isUiCapturedAt(ui_, *cursor_pos);
    }
    if (const auto cursor_delta = input.getCursorDelta(); cursor_delta.has_value()) {
        camera_input.cursor_delta_px = *cursor_delta;
    }
    if (const auto wheel_delta = input.getScrollOffset(); wheel_delta.has_value()) {
        camera_input.wheel_delta = *wheel_delta;
    }
    camera_input.text_input_focused = hasFocusedTextInput(ui_);
    camera_.update(camera_input);
};

ImageRef ModelTestLayer::render() {
    auto* image = render_target_.getImage(Color{clear_color_srgb_});

    renderModel(image, viewport_layout_);

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

void ModelTestLayer::renderModel(ImageRef target, const ModelTestViewportLayout& viewport_layout) {
    const vec2u render_extent = viewport_layout.render_extent;
    if (!target || render_extent.x == 0 || render_extent.y == 0 || !model_ || model_->getPrimitives().empty()) {
        MLE_I("ModelTest: renderModel skipped target={} model={} mesh_count={}", fmt::ptr(target), fmt::ptr(model_),
              model_ ? model_->getPrimitives().size() : 0);
        return;
    }

    auto& renderer = Renderer::i();
    const auto& meshes = model_->getPrimitives();
    const auto& model_skins = model_->getSkins();
    const auto& shader_descriptor = renderer_state_.currentDescriptor();
    const auto shader_float = [this](std::string_view mode_id, std::string_view parameter_id) {
        return std::get<f32>(renderer_state_.parameterValue(mode_id, parameter_id).value());
    };
    MLE_D("ModelTest: renderModel begin target={} viewport={}x{} mode={} meshes={} skins={}", fmt::ptr(target), render_extent.x, render_extent.y,
          shader_descriptor.display_name, meshes.size(), model_skins.size());

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

    GBuffer& gbuffer = getGBuffer(render_extent);
    ImageRef hdr_scene = getHdrSceneImage(render_extent);
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
    thread.setViewportAndScissor(Rectf{0.0F, 0.0F, as<f32>(render_extent.x), as<f32>(render_extent.y)});

    struct PushConstants {
        mat4f model;
        mat4f view_proj;
    } pc{};

    const mat4f preview_model = makeModelMatrix(meshes);
    const auto& camera = camera_.state();
    pc.view_proj = makeViewProj(render_extent, camera);
    const mat4f inv_view_proj = glm::inverse(pc.view_proj);

    const vec3f sun_dir = makeSunDirection(sun_yaw_, sun_pitch_);
    const f32 pitch_cos = std::cos(camera.pitch);
    const vec3f orbit_dir{
        std::sin(camera.yaw) * pitch_cos,
        std::sin(camera.pitch),
        std::cos(camera.yaw) * pitch_cos,
    };
    const vec3f camera_pos = camera.target + orbit_dir * camera.distance;

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
                  primitive.isTextured(), primitive.isSkinned(), primitive.getIndexCount(), shader_descriptor.wireframe);
            const Pipeline* pipeline = getModelTestGBufferPipeline(primitive.getVertexKind(), shader_descriptor.wireframe);
            thread.setPipeline(pipeline);
            if (shader_descriptor.wireframe) {
                thread.setLineWidth(shader_float("wireframe", "line_width"));
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

    if (scene_ != nullptr && held_item_model_ != nullptr && !held_item_model_->getPrimitives().empty() && !scene_->attachmentNode().empty()) {
        const usize attachment_node = scene_->attachmentNodeIndex();
        if (attachment_node != max<usize>() && attachment_node < node_globals_.size()) {
            std::vector<mat4f> held_item_node_globals(held_item_model_->getNodeCount());
            held_item_model_->evaluateBase(held_item_node_globals);

            const mat4f held_item_model = preview_model * scene_->heldItemTransform(node_globals_[attachment_node]);
            draw_model_meshes(held_item_model_->getPrimitives(), held_item_model, false, &held_item_node_globals);
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

    auto albedo_di = gbuffer.albedo->getDescriptorInfo();
    auto normal_di = gbuffer.normal->getDescriptorInfo();
    auto params_di = gbuffer.params->getDescriptorInfo();
    auto emissive_di = gbuffer.emissive->getDescriptorInfo();
    auto depth_di = gbuffer.depth->getDescriptorInfo();
    const auto resolve_settings = makeModelTestResolveSettings(renderer_state_, animation_time_);
    const auto composition_plan = makeModelTestCompositionPlan(renderer_state_, show_projection_, projection_color_srgb_);
    const auto composition_target = [hdr_scene, target](ModelTestCompositionTarget target_kind) -> ImageRef {
        switch (target_kind) {
            case ModelTestCompositionTarget::HDR_SCENE:
                return hdr_scene;
            case ModelTestCompositionTarget::OUTPUT:
                return target;
        }
        MLE_UNREACHABLE;
        return nullptr;
    };
    const auto composition_pipeline = [](const ModelTestCompositionPass& pass) -> const Pipeline* {
        switch (pass.pipeline) {
            case ModelTestCompositionPipeline::MODE_RESOLVE:
                return getModelTestResolvePipeline(pass.shader_mode);
            case ModelTestCompositionPipeline::FLAT_PROJECTION:
                return getModelTestProjectionPipeline(pass.blend);
            case ModelTestCompositionPipeline::TONEMAP:
                return getModelTestTonemapPipeline(pass.blend);
        }
        MLE_UNREACHABLE;
        return nullptr;
    };

    for (const ModelTestCompositionPass& pass : composition_plan.activePasses()) {
        AttachmentInfo color_attachment{};
        color_attachment.image = composition_target(pass.target);
        color_attachment.load_op = pass.kind == ModelTestCompositionPassKind::RESOLVE ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
        color_attachment.clear_value.color = vk::ClearColorValue{std::array{0.0F, 0.0F, 0.0F, 0.0F}};
        std::array color_attachments = {color_attachment};
        thread.setColorAttachments(color_attachments);
        thread.setDepthAttachment({});

        const Pipeline* pipeline = composition_pipeline(pass);
        switch (pass.kind) {
            case ModelTestCompositionPassKind::RESOLVE: {
                const auto& pass_shader_descriptor = modelTestShaderModeDescriptor(pass.shader_mode);
                MLE_D("ModelTest: begin resolve pass mode={}", pass_shader_descriptor.display_name);
                thread.beginRendering();
                thread.setViewportAndScissor(Rectf{0.0F, 0.0F, as<f32>(render_extent.x), as<f32>(render_extent.y)});
                thread.setPipeline(pipeline);

                if (pass_shader_descriptor.resolve_inputs == ModelTestResolveInputs::LIT) {
                    MLE_D("ModelTest: pushing lit resolve descriptors");
                    auto writes = pipeline->makeWrites(0, nullptr, &albedo_di, &normal_di, &params_di, &emissive_di, &depth_di, &lighting_di);
                    thread.pushDescriptor(0, writes);
                } else {
                    MLE_D("ModelTest: pushing debug resolve descriptors");
                    auto writes = pipeline->makeWrites(0, nullptr, &albedo_di, &normal_di, &depth_di);
                    thread.pushDescriptor(0, writes);
                }

                struct ResolvePushConstants {
                    mat4f inv_view_proj;
                    vec4f effect_params_0;
                    vec4f effect_params_1;
                    vec4f effect_params_2;
                } resolve_pc{
                    .inv_view_proj = inv_view_proj,
                    .effect_params_0 = resolve_settings.effect_params_0,
                    .effect_params_1 = resolve_settings.effect_params_1,
                    .effect_params_2 = resolve_settings.effect_params_2,
                };
                thread.pushConstants(&resolve_pc);
                MLE_D("ModelTest: draw resolve triangle");
                thread.draw(3, 1);

                if (pass_shader_descriptor.needs_outline) {
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
                    outline_pc.inv_extent = 1.0F / vec2f{render_extent};
                    outline_pc.depth_threshold = 0.00075F;
                    outline_pc.normal_threshold = shader_float("cartoon", "outline_normal_threshold");
                    outline_pc.alpha = 0.95F;
                    outline_pc.outline_width_px = shader_float("cartoon", "outline_width");
                    thread.pushConstants(&outline_pc);
                    MLE_D("ModelTest: draw outline triangle");
                    thread.draw(3, 1);
                }

                MLE_D("ModelTest: end resolve pass");
                thread.endRendering();
                MLE_D("ModelTest: transitioning HDR scene to FS_READ");
                hdr_scene->transitionState(thread.cmd(), Image::State::FS_READ);
                break;
            }
            case ModelTestCompositionPassKind::PROJECTION: {
                Polygon2f projection_polygon = model_->projectPolygon(Axis::Y);
                if (projection_epsilon_ > 0.0F) {
                    projection_polygon.simplify(projection_epsilon_);
                }

                const usize vertex_count = projection_polygon.vertexCount();
                if (vertex_count < 3) {
                    break;
                }
                std::vector<mat4f> base_node_globals(model_->getNodeCount());
                model_->evaluateBase(base_node_globals);
                f32 min_y = +FLT_MAX;
                for (const auto& node_primitive : meshes) {
                    mat4f transform = base_node_globals[node_primitive.node_index];
                    if (node_primitive.skin_index >= 0 && node_primitive.skin_index < as<int>(model_skins.size())) {
                        const auto& skin = model_skins[node_primitive.skin_index];
                        if (skin.jointCount() > 0) {
                            const auto& joint = skin.getJoints()[0];
                            transform = base_node_globals[joint.node_index] * joint.inverse_bind;
                        }
                    }
                    for (const vec3f& local_position : node_primitive.primitive.getCpuPositions()) {
                        min_y = std::min(min_y, vec3f{transform * vec4f{local_position, 1.0F}}.y);
                    }
                }
                if (min_y == +FLT_MAX) {
                    min_y = 0.0F;
                }

                std::vector<vec3f> projection_vertices(vertex_count);
                for (usize i = 0; i < vertex_count; ++i) {
                    const vec2f& vertex = projection_polygon.vertex(i);
                    projection_vertices[i] = vec3f{vertex.x, min_y - 0.01F, vertex.y};
                }

                BufferSlice projection_vertex_slice = frame_renderer.getHostVisibleBuffer(sizeof(vec3f) * vertex_count, vk::BufferUsageFlagBits::eVertexBuffer);
                projection_vertex_slice.buffer->write(projection_vertices.data(), projection_vertex_slice.size, projection_vertex_slice.offset);

                MLE_D("ModelTest: begin flat projection pass vertices={}", vertex_count);
                thread.beginRendering();
                thread.setViewportAndScissor(viewport_layout.target_rect.asF32());
                thread.setPipeline(pipeline);
                struct ProjectionPushConstants {
                    mat4f mvp;
                    vec4f color;
                } projection_pc{
                    .mvp = pc.view_proj * preview_model,
                    .color = pass.color,
                };
                thread.pushConstants(&projection_pc);
                thread.bindVertexBuffer(projection_vertex_slice.buffer, projection_vertex_slice.offset);
                thread.draw(as<u32>(vertex_count), 1);
                MLE_D("ModelTest: end flat projection pass");
                thread.endRendering();
                break;
            }
            case ModelTestCompositionPassKind::TONEMAP: {
                const auto& pass_shader_descriptor = modelTestShaderModeDescriptor(pass.shader_mode);
                MLE_D("ModelTest: begin HDR tonemap pass mode={}", pass_shader_descriptor.display_name);
                thread.beginRendering();
                thread.setViewportAndScissor(viewport_layout.target_rect.asF32());
                thread.setPipeline(pipeline);
                switch (pass.input) {
                    case ModelTestCompositionInput::HDR_SCENE: {
                        auto hdr_scene_di = hdr_scene->getDescriptorInfo();
                        auto writes = pipeline->makeWrites(0, nullptr, &hdr_scene_di);
                        thread.pushDescriptor(0, writes);
                        break;
                    }
                    case ModelTestCompositionInput::NONE:
                        MLE_UNREACHABLE;
                }

                struct TonemapPushConstants {
                    vec4f params;
                } tonemap_pc{
                    .params = vec4f{1.0F, pass.bypass_tonemap ? 1.0F : 0.0F, 0.0F, 0.0F},
                };
                thread.pushConstants(&tonemap_pc);
                MLE_D("ModelTest: draw tonemap triangle");
                thread.draw(3, 1);
                MLE_D("ModelTest: end HDR tonemap pass");
                thread.endRendering();
                break;
            }
        }
    }

    MLE_D("ModelTest: execute render commands");
    thread.executeCommands();
}

void ModelTestLayer::initializeScene() {
    const Path models_root = Path{ResPath::RES} / ResPath::MODELS;
    ModelTestAssets assets{models_root};
    scene_ = std::make_unique<ModelTestScene>(
        std::move(assets),
        [models_root](std::string_view path, usize root_node) -> std::expected<ModelTestLoadedMesh, std::string> {
            const std::string cache_key = fmt::format("model_test:{}#node_{}", path, root_node);
            auto& cache = Renderer::i().meshCache();
            MeshRef mesh = cache.get(makeAssetId(cache_key));
            if (mesh == nullptr) {
                GLTF gltf;
                const Path absolute_path = models_root / Path{path};
                if (gltf.load(absolute_path) != Result::OK) {
                    return std::unexpected{"Failed to load model resource " + std::string{path}};
                }
                mesh = cache.add(makeAssetId(cache_key), gltf, root_node);
            }
            if (mesh == nullptr) {
                return std::unexpected{"Failed to create model resource " + std::string{path}};
            }

            const bool skinned = std::ranges::any_of(mesh->getPrimitives(), [](const auto& node_primitive) { return node_primitive.primitive.isSkinned(); });
            std::vector<std::pair<std::string, usize>> included_nodes;
            const auto& mesh_nodes = mesh->getNodes();
            included_nodes.reserve(mesh_nodes.size());
            for (usize index = 0; index < mesh_nodes.size(); ++index) {
                const auto& node = mesh_nodes[index];
                if (node.included && !node.name.empty()) {
                    included_nodes.emplace_back(node.name, index);
                }
            }
            return ModelTestLoadedMesh{.resource = mesh, .skinned = skinned, .included_nodes = std::move(included_nodes)};
        },
        [models_root](std::string_view path, std::string_view selector) -> std::expected<AnimationClipRef, std::string> {
            auto& cache = Renderer::i().animationCache();
            const entt::id_type id = AnimationCache::makeAnimationId(path, selector);
            if (AnimationClipRef clip = cache.get(id); clip != nullptr) {
                return clip;
            }

            GLTF gltf;
            const Path absolute_path = models_root / Path{path};
            if (gltf.load(absolute_path) != Result::OK) {
                return std::unexpected{"Failed to load animation resource " + std::string{path}};
            }
            AnimationClipRef clip = cache.addAnimation(path, gltf, selector);
            if (clip == nullptr) {
                return std::unexpected{"Failed to create animation '" + std::string{selector} + "' from " + std::string{path}};
            }
            return clip;
        });

    if (const auto paths = scene_->startup(); !paths.has_value()) {
        MLE_W("ModelTestLayer resource scan failed: {}", paths.error());
    }
}

bool ModelTestLayer::refreshResourcePaths() {
    if (scene_ == nullptr) {
        return false;
    }
    const auto paths = scene_->startup();
    if (!paths.has_value()) {
        MLE_W("ModelTestLayer resource scan failed: {}", paths.error());
    }
    return paths.has_value();
}

sol::table ModelTestLayer::completeResourceForLua(ModelResourceKind kind, const std::string& query) {
    auto table = Client::i().lua().createTable();
    if (scene_ == nullptr) {
        table["replacement"] = query;
        table["message"] = std::string{"Resource state unavailable"};
        table["suggestions"] = Client::i().lua().createTable();
        return table;
    }

    const CompletionResult result = scene_->complete(kind, query);
    table["replacement"] = result.replacement;
    table["message"] = result.message;
    auto suggestions = Client::i().lua().createTable();
    for (usize i = 0; i < result.suggestions.size(); ++i) {
        suggestions[i + 1] = result.suggestions[i];
    }
    table["suggestions"] = suggestions;
    return table;
}

bool ModelTestLayer::submitModel(const std::string& resource_id) {
    if (scene_ == nullptr || !scene_->submitModel(resource_id)) {
        MLE_W("ModelTestLayer model submit failed: {}", scene_ != nullptr ? scene_->status() : "resource state unavailable");
        return false;
    }

    model_ = scene_->model()->resource;
    node_globals_.clear();
    skin_mats_.clear();
    animation_time_ = 0.0F;

    if (current_animation_ != nullptr && !animationTargetsModel(current_animation_, model_)) {
        MLE_W("ModelTestLayer animation has no channels matching model '{}'; base pose will be used", scene_->model()->id);
    }
    return true;
}

bool ModelTestLayer::submitHeldItem(const std::string& resource_id) {
    if (scene_ == nullptr || !scene_->submitHeldItem(resource_id)) {
        MLE_W("ModelTestLayer held-item submit failed: {}", scene_ != nullptr ? scene_->status() : "resource state unavailable");
        return false;
    }
    held_item_model_ = scene_->heldItem()->resource;
    return true;
}

bool ModelTestLayer::submitAnimation(const std::string& resource_id) {
    if (scene_ == nullptr || !scene_->submitAnimation(resource_id)) {
        MLE_W("ModelTestLayer animation submit failed: {}", scene_ != nullptr ? scene_->status() : "resource state unavailable");
        return false;
    }
    current_animation_ = scene_->animation()->resource;
    animation_time_ = 0.0F;
    if (model_ != nullptr && !animationTargetsModel(current_animation_, model_)) {
        MLE_W("ModelTestLayer animation '{}' has no channels matching model '{}'; base pose will be used", scene_->animation()->id, scene_->model()->id);
    }
    return true;
}

bool ModelTestLayer::submitAttachment(const std::string& selector) {
    if (scene_ == nullptr || !scene_->submitAttachment(selector)) {
        MLE_W("ModelTestLayer attachment submit failed: {}", scene_ != nullptr ? scene_->status() : "resource state unavailable");
        return false;
    }
    return true;
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

void ModelTestLayer::setHeldItemTranslation(f32 x, f32 y, f32 z) {
    if (scene_ != nullptr) {
        scene_->setHeldItemTranslation(vec3f{x, y, z});
    }
}

void ModelTestLayer::setHeldItemRotation(f32 x, f32 y, f32 z) {
    if (scene_ != nullptr) {
        scene_->setHeldItemRotation(vec3f{x, y, z});
    }
}

void ModelTestLayer::setHeldItemScale(f32 value) {
    if (scene_ != nullptr) {
        scene_->setHeldItemScale(value);
    }
}

void ModelTestLayer::clearHeldItem() {
    if (scene_ != nullptr) {
        scene_->clearHeldItem();
    }
    held_item_model_ = nullptr;
}

void ModelTestLayer::clearAnimation() {
    if (scene_ != nullptr) {
        scene_->clearAnimation();
    }
    current_animation_ = nullptr;
    animation_time_ = 0.0F;
}
}  // namespace mle::user
