#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ModelTestCamera.h"
#include "ModelTestRenderer.h"
#include "ModelTestScene.h"
#include "mle/client/Layer.h"
#include "mle/ui/UI.h"
#include "mle/utils/ECS.h"

namespace mle::user {
enum class ModelTestBackgroundMode : u8 { CLEAR_COLOR, CUBEMAP };

class ModelTestLayer : public mle::client::Layer {
  public:
    MLE_NO_COPY_MOVE(ModelTestLayer)

    ModelTestLayer() = default;
    ~ModelTestLayer() override = default;

    void init() override;
    void shutdown() override;

    void update() override;
    ImageRef render() override;

  private:
    GBuffer& getGBuffer(vec2u size);
    ImageRef getHdrSceneImage(vec2u size);
    void renderModel(ImageRef target, const ModelTestViewportLayout& viewport_layout);
    void initializeScene();
    bool refreshResourcePaths();
    bool refreshCubemapPaths();
    [[nodiscard]] sol::table completeResourceForLua(ModelResourceKind kind, const std::string& query);
    [[nodiscard]] sol::table completeCubemapForLua(const std::string& query);
    bool setBackgroundMode(const std::string& mode);
    bool submitModel(const std::string& resource_id);
    bool submitHeldItem(const std::string& resource_id);
    bool submitAnimation(const std::string& resource_id);
    bool submitAttachment(const std::string& selector);
    void setSunYaw01(f32 value);
    void setSunPitch01(f32 value);
    void setSunIntensity01(f32 value);
    void setAmbient01(f32 value);
    void setHeldItemTranslation(f32 x, f32 y, f32 z);
    void setHeldItemRotation(f32 x, f32 y, f32 z);
    void setHeldItemScale(f32 value);
    void clearHeldItem();
    void clearAnimation();
    bool submitCubemap(const std::string& name);
    void clearCubemap();

    std::unique_ptr<ModelTestScene> scene_;
    MeshRef model_ = nullptr;
    MeshRef held_item_model_ = nullptr;
    AnimationClipRef current_animation_ = nullptr;
    std::vector<mat4f> node_globals_;
    std::unordered_map<int, std::vector<mat4f>> skin_mats_;
    f32 animation_time_ = 0.0F;
    ModelTestCamera camera_;
    ModelTestViewportLayout viewport_layout_{};
    f32 sun_yaw_ = -0.610865F;
    f32 sun_pitch_ = 0.785398F;
    f32 sun_intensity_ = 2.0F;
    f32 ambient_ = 0.08F;
    ModelTestRendererState renderer_state_;
    bool show_projection_ = false;
    f32 projection_epsilon_ = 0.0f;
    vec4f clear_color_srgb_{1.0F};
    vec4f projection_color_srgb_{0.0F, 0.8F, 1.0F, 1.0F};
    std::mutex background_mutex_;
    ModelTestBackgroundMode background_mode_ = ModelTestBackgroundMode::CLEAR_COLOR;
    std::string selected_cubemap_;
    ImageHnd cubemap_image_;
    std::vector<std::string> cubemap_names_;
    std::string background_status_;
    std::vector<std::string> cubemap_completion_candidates_;
    std::string cubemap_completion_initial_query_;
    std::string cubemap_completion_last_output_;
    usize cubemap_completion_index_ = 0;
    bool cubemap_completion_started_ = false;

    UI ui_;

    mle::client::WindowSizedRenderTarget render_target_;
    std::array<GBuffer, 2> gbuffers_;
    std::array<ImageHnd, 2> hdr_scene_images_;
};
}  // namespace mle::user
