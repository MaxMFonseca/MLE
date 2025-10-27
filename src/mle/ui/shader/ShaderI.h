#pragma once

#include "../Types.h"
#include "mle/math/Types.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/utils/ECS.h"

namespace mle::ui {
struct ShaderPacketI {
    ShaderPacketI() = default;
    virtual ~ShaderPacketI() = default;

    MLE_NO_COPY_MOVE(ShaderPacketI);

    usize version = max<usize>();

    virtual void render(CompRenderingCtx& ctx) = 0;
};

struct ShaderI {
    ShaderI() = default;
    virtual ~ShaderI() = default;

    MLE_NO_COPY_MOVE(ShaderI);

    bool before_children = false;
    bool dedicate_render_target = false;
    Color clear_color = Color::ZERO;

    usize version = 0;
    void versionUp() { ++version; }

    virtual void set(const sol::object& obj) = 0;
    void setBase(const sol::table& table);

    virtual void doUpdatePacket(ShaderPacketI* packet) = 0;

    [[nodiscard]] virtual entt::id_type getType() const { return entt::hashed_string{"ShaderI"}; }
};
}  // namespace mle::ui
