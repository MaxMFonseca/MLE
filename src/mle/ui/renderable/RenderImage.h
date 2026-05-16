#pragma once

#include "mle/ui/renderable/RenderableI.h"

namespace mle::ui::renderable {
struct RenderImagePacket : public RenderablePacketI {
    RenderImagePacket() = default;
    ~RenderImagePacket() override = default;

    MLE_NO_COPY_MOVE(RenderImagePacket);

    Color color = Color::ONE;
    bool flip_x = false;
    bool flip_y = false;

    void render(CompRenderingCtx& ctx) override;
};

struct RenderImage : public RenderableI {
    Color color = Color::ONE;
    bool flip_x = false;
    bool flip_y = false;

    void setColor(const Color& c);
    void setColor(const sol::object& obj) { setColor(Color::fromLua(obj)); }
    void setFlipX(const sol::object& obj);
    void setFlipY(const sol::object& obj);

    void set(const Entt& e, const sol::object& obj) override;
    [[nodiscard]] vec2u calculateBounds(const Entt& e, vec2u max_size) override;

    [[nodiscard]] entt::id_type getType() const override { return type(); }
    static entt::id_type type() { return entt::hashed_string{"RenderImage"}; }

    void doUpdatePacket(const Entt& ew, RenderablePacketI* packet) override;

    static void apply(const Entt& e, const sol::object& obj);
};
}  // namespace mle::ui::renderable
