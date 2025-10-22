#pragma once

#include "mle/ui/renderable/RenderableI.h"
#include "mle/utils/ECS.h"
namespace mle::ui::renderable {
struct Sprite : public RenderableI {
    ImageRef image;
    entt::id_type texture_id{};

    void setTexture(const std::string& src);

    void set(const sol::object& obj) override;
    [[nodiscard]] vec2u calculateBounds([[maybe_unused]] vec2u max_size) const override { return {}; };

    [[nodiscard]] entt::hashed_string getType() const override { return type(); }
    static entt::hashed_string type() { return {"Sprite"}; }

    [[nodiscard]] std::unique_ptr<RenderableI> clone() const override { return std::make_unique<Sprite>(*this); }

    void render(Ctx& ctx) override;

    static void apply(const Entt& e, const sol::object& obj);
};
}  // namespace mle::ui::renderable
