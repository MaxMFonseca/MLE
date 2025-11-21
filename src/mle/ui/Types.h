#pragma once

#include <functional>

#include "mle/lua/Lua.h"
#include "mle/lua/Types.h"
#include "mle/math/Types.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/utils/ECS.h"

namespace mle {
class UI;

namespace ui {
class Entt;

using HoverFn = std::move_only_function<void(const Entt& e)>;
enum class HoverState : u8 { NO, IN, HOVER, OUT };

struct CompRenderingCtx {
    RenderingThread& thread;
    Lua& lua;
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

namespace system {
using EventCallbackFunction = std::function<void(const sol::object& obj)>;
class Events;
class EventListener;
using EventListenerRef = EventListener*;
using EventListenerHnd = std::unique_ptr<EventListener>;

}  // namespace system

}  // namespace ui
}  // namespace mle

namespace fmt {
template <>
struct formatter<mle::ui::HoverState> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::HoverState v, FormatContext& ctx) const {
        switch (v) {
            case mle::ui::HoverState::NO:
                return format_to(ctx.out(), "NO");
            case mle::ui::HoverState::IN:
                return format_to(ctx.out(), "IN");
            case mle::ui::HoverState::HOVER:
                return format_to(ctx.out(), "HOVER");
            case mle::ui::HoverState::OUT:
                return format_to(ctx.out(), "OUT");
        }
    }
};
}  // namespace fmt
