#pragma once

#include "../Types.h"
#include "mle/math/Types.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/utils/ECS.h"

namespace mle::ui {
struct RenderableI {
    struct Ctx {
        RenderingThread& thread;
        vec2u size;
    };

    RenderableI() = default;
    virtual ~RenderableI() = default;

    RenderableI(RenderableI&&) = default;
    RenderableI& operator=(RenderableI&&) = default;
    RenderableI(const RenderableI&) = default;
    RenderableI& operator=(const RenderableI&) = default;

    virtual void set(const sol::object& obj) = 0;
    [[nodiscard]] virtual std::unique_ptr<RenderableI> clone() const = 0;
    [[nodiscard]] virtual vec2u calculateBounds(vec2u max_size) const = 0;
    [[nodiscard]] virtual entt::hashed_string getType() const { return {"RenderableI"}; }

    virtual void render(Ctx& ctx) = 0;
};
}  // namespace mle::ui
