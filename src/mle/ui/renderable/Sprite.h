#pragma once

#include "mle/ui/renderable/RenderableI.h"
#include "mle/utils/ECS.h"
namespace mle::ui::renderable {
enum class SpriteSource : u8 { TEXTURE, IMAGE };

[[nodiscard]] const Pipeline* getSpritePipeline();

struct SpritePacket : public RenderablePacketI {
    SpritePacket() = default;
    ~SpritePacket() override = default;

    MLE_NO_COPY_MOVE(SpritePacket);

    ImageRef image{};
    Color color = Color::ONE;
    SpriteSource source = SpriteSource::TEXTURE;
    ImageRef source_image{};
    entt::id_type texture_id{};
    bool source_changed = false;
    bool flip_x = false;
    bool flip_y = false;
    vec2f uv = {0.0F, 0.0F};
    vec2f uv_size = {1.0F, 1.0F};

    void render(CompRenderingCtx& ctx) override;
};

struct Sprite : public RenderableI {
    SpriteSource source = SpriteSource::TEXTURE;
    ImageRef image{};
    entt::id_type texture_id{};
    Color color = Color::ONE;
    bool flip_x = false;
    bool flip_y = false;
    vec2f uv = {0.0F, 0.0F};
    vec2f uv_size = {1.0F, 1.0F};

    bool fit = false;

    void setTexture(const Entt& ew, const std::string& src);
    void setImage(const Entt& ew, ImageRef image);
    void setColor(const Color& c);
    void setColor(const sol::object& obj) { setColor(Color::fromLua(obj)); }
    void setFit(const sol::object& obj);
    void setFlipX(const sol::object& obj);
    void setFlipY(const sol::object& obj);
    void setUv(const Entt& ew, const sol::object& obj);
    void setUvSize(const Entt& ew, const sol::object& obj);

    void set(const Entt& e, const sol::object& obj) override;
    [[nodiscard]] vec2u calculateBounds(const Entt& e, [[maybe_unused]] vec2u max_size) override;

    [[nodiscard]] entt::id_type getType() const override { return type(); }
    static entt::id_type type() { return entt::hashed_string{"Sprite"}; }

    void doUpdatePacket(const Entt& ew, RenderablePacketI* packet) override;

    static void apply(const Entt& e, const sol::object& obj);
};
}  // namespace mle::ui::renderable
