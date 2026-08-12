#include "ModelTestCamera.h"

#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>

namespace mle::user {
namespace {
constexpr f32 ORBIT_RADIANS_PER_PIXEL = 0.01F;
constexpr f32 ZOOM_EXPONENT_PER_STEP = 0.2F;
constexpr f32 HALF_VERTICAL_FOV_RADIANS = 0.39269908F;
constexpr vec3f WORLD_UP{0.0F, 1.0F, 0.0F};
}  // namespace

bool ModelTestViewportLayout::containsCursor(vec2f cursor_pos) const {
    return cursor_pos.x >= as<f32>(target_rect.left()) && cursor_pos.x < as<f32>(target_rect.right()) && cursor_pos.y >= as<f32>(target_rect.top()) &&
           cursor_pos.y < as<f32>(target_rect.bottom());
}

ModelTestViewportLayout resolveModelTestViewportLayout(Recti viewport_bounds, vec2u root_extent) {
    const Recti root_rect{0, 0, as<i32>(root_extent.x), as<i32>(root_extent.y)};
    const Recti target_rect = viewport_bounds.intersection(root_rect);
    return {
        .target_rect = target_rect,
        .render_extent = vec2u{target_rect.size()},
    };
}

void ModelTestCamera::update(const ModelTestCameraInput& input) {
    if (!input.left_down) {
        left_captured_ = false;
    }
    if (!input.middle_down) {
        middle_captured_ = false;
    }

    const bool actions_blocked = input.ui_captured || input.text_input_focused;
    bool acquired = false;
    if (input.cursor_inside_viewport && !actions_blocked) {
        if (input.left_pressed && input.left_down) {
            left_captured_ = true;
            acquired = true;
        }
        if (input.middle_pressed && input.middle_down) {
            middle_captured_ = true;
            acquired = true;
        }
    }
    if (acquired) {
        return;
    }

    if (!actions_blocked) {
        if (left_captured_ && input.left_down) {
            state_.yaw += input.cursor_delta_px.x * ORBIT_RADIANS_PER_PIXEL;
            state_.pitch = std::clamp(state_.pitch + input.cursor_delta_px.y * ORBIT_RADIANS_PER_PIXEL, -MAX_PITCH, MAX_PITCH);
        } else if (middle_captured_ && input.middle_down && input.viewport_size_px.y > 0.0F) {
            const f32 pitch_cos = std::cos(state_.pitch);
            const vec3f orbit_direction{
                std::sin(state_.yaw) * pitch_cos,
                std::sin(state_.pitch),
                std::cos(state_.yaw) * pitch_cos,
            };
            const vec3f camera_right = glm::normalize(glm::cross(WORLD_UP, orbit_direction));
            const vec3f camera_up = glm::normalize(glm::cross(orbit_direction, camera_right));
            const f32 world_units_per_pixel = 2.0F * state_.distance * std::tan(HALF_VERTICAL_FOV_RADIANS) / input.viewport_size_px.y;
            state_.target -= camera_right * input.cursor_delta_px.x * world_units_per_pixel;
            state_.target += camera_up * input.cursor_delta_px.y * world_units_per_pixel;
        }
    }

    if (input.cursor_inside_viewport && !actions_blocked && input.wheel_delta != 0.0F) {
        state_.distance = std::clamp(state_.distance * std::exp(-input.wheel_delta * ZOOM_EXPONENT_PER_STEP), MIN_DISTANCE, MAX_DISTANCE);
    }
}

void ModelTestCamera::reset() {
    state_ = {};
    left_captured_ = false;
    middle_captured_ = false;
}

void ModelTestCamera::setState(ModelTestCameraState state) {
    state.pitch = std::clamp(state.pitch, -MAX_PITCH, MAX_PITCH);
    state.distance = std::clamp(state.distance, MIN_DISTANCE, MAX_DISTANCE);
    state_ = state;
}
}  // namespace mle::user
