#pragma once

#include <optional>
#include <vector>

#include "mle/ui/components/Bounds.h"
#include "mle/utils/Color.h"
#include "sol/forward.hpp"

namespace mle::ui {
class Entt;
}

namespace mle::ui::comp {
enum class AnimationEase : u8 {
    LINEAR,
    IN_QUAD,
    OUT_QUAD,
    IN_OUT_QUAD,
};

enum class AnimationTarget : u8 {
    POS_X,
    POS_Y,
    SIZE_X,
    SIZE_Y,
    RENDER_SCALE,
    BACKGROUND,
    BORDER_COLOR,
};

enum class AnimationValueKind : u8 {
    NUMBER,
    TARGET_BOUND,
    COLOR,
};

struct AnimationValue {
    AnimationValueKind kind = AnimationValueKind::NUMBER;
    f32 number = 0.0F;
    TargetBound target_bound{};
    Color color = Color::ZERO;
};

struct AnimationTrack {
    AnimationTarget target = AnimationTarget::RENDER_SCALE;
    AnimationValue from{};
    AnimationValue to{};
    f32 duration = 0.0F;
    f32 delay = 0.0F;
    bool loop = false;
    bool yoyo = false;
    AnimationEase ease = AnimationEase::LINEAR;
};

struct SpriteAnimation {
    vec2u frame_size{};
    u32 frames = 0;
    f32 fps = 0.0F;
    bool loop = true;
};

struct TypewriterAnimation {
    f32 cps = 0.0F;
    f32 start_delay = 0.0F;
};

struct Animation {
    f32 elapsed = 0.0F;
    bool loop = false;
    std::vector<AnimationTrack> tracks;
    std::optional<SpriteAnimation> sprite;
    std::optional<TypewriterAnimation> typewriter;

    void set(const Entt& ew, const sol::object& obj);
    void tick(const Entt& ew, f32 dt);

    static f32 ease(AnimationEase ease, f32 t);
    static void apply(const Entt& ew, const sol::object& obj);
};
}  // namespace mle::ui::comp
