#pragma once

#include "mle/math/Types2D.h"

namespace mle::user {
struct ModelTestViewportLayout {
    Recti target_rect{};
    vec2u render_extent{};

    [[nodiscard]] bool containsCursor(vec2f cursor_pos) const;
};

[[nodiscard]] ModelTestViewportLayout resolveModelTestViewportLayout(Recti viewport_bounds, vec2u root_extent);

struct ModelTestCameraState {
    f32 yaw = 0.0F;
    f32 pitch = 0.0F;
    f32 distance = 10.0F;
    vec3f target{0.0F, 0.15F, 0.0F};
};

struct ModelTestCameraInput {
    vec2f cursor_delta_px{};
    vec2f viewport_size_px{1.0F, 1.0F};
    bool cursor_inside_viewport = false;
    bool ui_captured = false;
    bool text_input_focused = false;
    bool left_pressed = false;
    bool left_down = false;
    bool middle_pressed = false;
    bool middle_down = false;
    f32 wheel_delta = 0.0F;
};

class ModelTestCamera {
  public:
    static constexpr f32 MAX_PITCH = 1.22173048F;
    static constexpr f32 MIN_DISTANCE = 0.25F;
    static constexpr f32 MAX_DISTANCE = 100.0F;

    void update(const ModelTestCameraInput& input);
    void reset();
    void setState(ModelTestCameraState state);

    [[nodiscard]] const ModelTestCameraState& state() const { return state_; }

  private:
    ModelTestCameraState state_{};
    bool left_captured_ = false;
    bool middle_captured_ = false;
};
}  // namespace mle::user
