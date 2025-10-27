#pragma once

#include "../Types.h"
#include "mle/utils/Color.h"

namespace mle::ui::comp {
struct Name {
    std::string o;
};

struct Background {
    Color color = Color::ZERO;

    void set(const sol::object& obj) { color = Color::fromLua(obj); }

    static void apply(const Entt& e, const sol::object& obj);
};

// TODO: add unique ID(name) component that maps on the UI base

struct RequestInternalBoundsUpdateFlag {};
struct RequestExternalBoundsUpdateFlag {};

}  // namespace mle::ui::comp
