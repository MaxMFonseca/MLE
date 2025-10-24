#pragma once

#include "../Types.h"
#include "mle/math/Types.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/utils/AtmoicTripleBuffer.h"
#include "mle/utils/ECS.h"

namespace mle::ui {
struct RenderablePacketI {
    RenderablePacketI() = default;
    virtual ~RenderablePacketI() = default;

    MLE_NO_COPY_MOVE(RenderablePacketI);

    usize version = max<usize>();

    virtual void render(CompRenderingCtx& ctx) = 0;
};

struct RenderableI {
    RenderableI() = default;
    virtual ~RenderableI() = default;

    MLE_NO_COPY_MOVE(RenderableI);

    usize version = 0;
    void versionUp() { ++version; }

    virtual void set(const sol::object& obj) = 0;

    void updatePacket(RenderablePacketI* packet) {
        if (packet && packet->version != version) {
            packet->version = version;
            doUpdatePacket(packet);
        }
    }

    virtual void doUpdatePacket(RenderablePacketI* packet) = 0;

    [[nodiscard]] virtual vec2u calculateBounds(const Entt& e, vec2u max_size) = 0;

    [[nodiscard]] virtual entt::id_type getType() const { return entt::hashed_string{"RenderableI"}; }
};
}  // namespace mle::ui
