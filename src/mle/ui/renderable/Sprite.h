#pragma once

#include "mle/ui/renderable/RenderableI.h"
#include "mle/utils/ECS.h"
namespace mle::ui::renderable {
struct SpritePacket : public RenderablePacketI {
    SpritePacket() = default;
    ~SpritePacket() override = default;

    MLE_NO_COPY_MOVE(SpritePacket);

    ImageRef image{};
    Color color = Color::ONE;
    entt::id_type texture_id{};
    bool texture_id_changed = false;

    void render(Ctx& ctx) override;
};

struct Sprite : public RenderableI {
    entt::id_type texture_id{};
    Color color = Color::ONE;

    void setTexture(const std::string& src);

    void set(const sol::object& obj) override;
    [[nodiscard]] vec2u calculateBounds(const Entt& e, [[maybe_unused]] vec2u max_size) override;

    [[nodiscard]] entt::id_type getType() const override { return type(); }
    static entt::id_type type() { return entt::hashed_string{"Sprite"}; }

    [[nodiscard]] std::unique_ptr<RenderablePacketI> createPacket() const override { return std::make_unique<SpritePacket>(); }
    void doUpdatePacket(RenderablePacketI* packet) override;

    static void apply(const Entt& e, const sol::object& obj);
};
}  // namespace mle::ui::renderable
