#pragma once

#include "mle/lua/Types.h"
#include "mle/math/Types.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/utils/ECS.h"

namespace mle {
class UI;

namespace ui {
class Entt;

struct CompRenderingCtx {
    RenderingThread& thread;
    vec4i rounding_corners_radius_px;
};

struct PaddingPx {
    int t = 0, b = 0, l = 0, r = 0;

    [[nodiscard]] vec2i removeFrom(vec2i size) const;
};

namespace comp {
class Relationship;
struct Name;
struct Bounds;
struct Border;
class Container;
}  // namespace comp
}  // namespace ui
}  // namespace mle
