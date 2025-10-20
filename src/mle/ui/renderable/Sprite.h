#pragma once

#include "mle/ui/renderable/RenderableI.h"
namespace mle::ui::renderable {
struct Sprite : public RenderableI {
    void set([[maybe_unused]] const sol::object& obj) override {};
    [[nodiscard]] vec2u calculateBounds([[maybe_unused]] vec2u max_size) const override { return {}; };

    [[nodiscard]] entt::hashed_string getType() const override { return type(); }
    static entt::hashed_string type() { return {"Sprite"}; }

    [[nodiscard]] std::unique_ptr<RenderableI> clone() const override { return std::make_unique<Sprite>(*this); }

    static void apply(const Entt& e, const sol::object& obj);
};
}  // namespace mle::ui::renderable
