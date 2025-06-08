#pragma once

#include <entt/entt.hpp>

#include "mle/common/Color.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"

namespace mle::ui::detail::e_components {
struct Base {
    entt::entity parent;
};

struct Name {
    std::string name;

    static void onLuaKeyName(entt::registry& reg, entt::entity e, const sol::object& obj);
};

struct Position {
    vec2f pos;

    struct {
        vec2f pos;
    } target;
};

struct Padding {};
struct Margin {};
struct Size {};

struct Background {
    Color color;
};

struct Renderable {};

class RootImage {
  public:
    MLE_NO_COPY_MOVE(RootImage)

    explicit RootImage(const sol::table& table);
    ~RootImage() = default;

    [[nodiscard]] auto getImage() { return image_.get(); }

    [[nodiscard]] const auto& getClearColor() const { return clear_color_; }
    [[nodiscard]] auto getClearColorVk() const { return renderer::toVkClearColor(clear_color_); }

  private:
    renderer::ImageHnd image_;
    Color clear_color_ = Color::ZERO;
};
}  // namespace mle::ui::detail::e_components
