#pragma once

#include <array>
#include <string>
#include <vector>

#include "mle/client/Layer.h"
#include "mle/renderer/SkinBinding.h"
#include "mle/ui/UI.h"

namespace mle::user {
class ModelTestLayer : public mle::client::Layer {
  public:
    MLE_NO_COPY_MOVE(ModelTestLayer)

    ModelTestLayer() = default;
    ~ModelTestLayer() override = default;

    void init() override;
    void shutdown() override;

    void update() override;
    ImageRef render() override;

    ImageRef getImage();

  private:
    ImageRef getDepthImage(vec2u size);
    void renderModel(ImageRef target);
    void setCameraYaw01(f32 value);
    void setCameraPitch01(f32 value);
    void setAnimation(const std::string& name);

    ModelRef model_ = nullptr;
    SkeletonRef skeleton_ = nullptr;
    AnimationClipRef current_animation_ = nullptr;
    std::vector<AnimationClipRef> animations_;
    SkinBinding skin_binding_{};
    std::vector<mat4f> node_globals_;
    std::vector<mat4f> skin_mats_;
    f32 animation_time_ = 0.0F;
    f32 camera_yaw_ = 0.0F;
    f32 camera_pitch_ = 0.0F;

    UI ui_;

    std::array<ImageHnd, 2> images_;
    std::array<ImageHnd, 2> depth_images_;
};
}  // namespace mle::user
