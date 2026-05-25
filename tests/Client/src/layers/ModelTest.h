#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include "mle/client/Layer.h"
#include "mle/ui/UI.h"
#include "mle/utils/ECS.h"

namespace mle::user {
enum class ModelTestShaderMode : u8 {
    PBR = 0,
    CARTOON,
    WIREFRAME,
    NORMALS,
    ALBEDO,
    COUNT,
};

class ModelTestLayer : public mle::client::Layer {
  public:
    MLE_NO_COPY_MOVE(ModelTestLayer)

    ModelTestLayer() = default;
    ~ModelTestLayer() override = default;

    void init() override;
    void shutdown() override;

    void update() override;
    ImageRef render() override;

    struct ModelOption {
        std::string key;
        std::string file;
        usize root_node = max<usize>();
    };

  private:
    struct AnimationOption {
        std::string label;
        entt::id_type id{};
    };

    GBuffer& getGBuffer(vec2u size);
    ImageRef getHdrSceneImage(vec2u size);
    void renderModel(ImageRef target);
    void refreshAssets();
    [[nodiscard]] sol::table refreshAssetsForLua();
    [[nodiscard]] sol::table makeModelNamesTable() const;
    [[nodiscard]] sol::table makeAnimationNamesTable() const;
    [[nodiscard]] sol::table makeHeldItemNamesTable() const;
    [[nodiscard]] sol::table makeShaderModeNamesTable() const;
    bool setModel(const std::string& name);
    bool setHeldItem(const std::string& name);
    void setShaderMode(const std::string& name);
    void setCameraYaw01(f32 value);
    void setCameraPitch01(f32 value);
    void setCameraDistance01(f32 value);
    void setSunYaw01(f32 value);
    void setSunPitch01(f32 value);
    void setSunIntensity01(f32 value);
    void setAmbient01(f32 value);
    void setOutlineWidth01(f32 value);
    void setToonBandSoftness01(f32 value);
    void setToonShadowLevel01(f32 value);
    void setToonMidLevel01(f32 value);
    void setToonHighlightLevel01(f32 value);
    void setToonSpecStrength01(f32 value);
    void setToonRimStrength01(f32 value);
    void setWireframeWidth01(f32 value);
    void setHeldItemScale01(f32 value);
    void setAnimation(const std::string& name);
    void clearAnimation();

    ModelRef model_ = nullptr;
    ModelRef held_item_model_ = nullptr;
    AnimationClipRef current_animation_ = nullptr;
    std::vector<entt::id_type> model_ids_;
    std::vector<ModelOption> model_options_;
    std::vector<ModelOption> held_item_options_;
    std::vector<AnimationOption> animation_options_;
    std::vector<std::string> animation_names_;
    std::vector<std::string> model_files_;
    std::vector<std::string> animation_files_;
    std::string current_model_name_;
    std::string current_held_item_name_;
    std::string current_animation_name_;
    std::string held_item_attachment_warning_key_;
    std::vector<mat4f> node_globals_;
    std::unordered_map<int, std::vector<mat4f>> skin_mats_;
    f32 animation_time_ = 0.0F;
    f32 camera_yaw_ = 0.0F;
    f32 camera_pitch_ = 0.0F;
    f32 camera_distance_ = 10.0F;
    f32 sun_yaw_ = -0.610865F;
    f32 sun_pitch_ = 0.785398F;
    f32 sun_intensity_ = 2.0F;
    f32 ambient_ = 0.08F;
    f32 outline_width_px_ = 2.5F;
    f32 toon_band_softness_ = 0.01F;
    f32 toon_shadow_level_ = 0.18F;
    f32 toon_mid_level_ = 0.72F;
    f32 toon_highlight_level_ = 1.0F;
    f32 toon_spec_strength_ = 1.0F;
    f32 toon_rim_strength_ = 0.4F;
    f32 wireframe_width_ = 1.5F;
    f32 held_item_scale_ = 1.0F;
    ModelTestShaderMode shader_mode_ = ModelTestShaderMode::PBR;

    UI ui_;

    mle::client::WindowSizedRenderTarget render_target_;
    std::array<GBuffer, 2> gbuffers_;
    std::array<ImageHnd, 2> hdr_scene_images_;
};
}  // namespace mle::user
