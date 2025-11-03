#pragma once

#include "../Types.h"
#include "mle/utils/Color.h"
#include "sol/forward.hpp"

namespace mle::ui::comp {
struct Name {
    std::string o;
};

struct ID {
    std::string o;

    static void apply(const Entt& e, const sol::object& obj);
};

struct Background {
    Color color = Color::ZERO;

    void set(const sol::object& obj) { color = Color::fromLua(obj); }

    static void apply(const Entt& e, const sol::object& obj);
};

struct Table {
    sol::table o;

    static void apply(const Entt& e, const sol::object& obj);

    static sol::object get(const Entt& e);
};

struct OnUpdate {
    using FuncType = std::function<void(const Entt& e)>;

    FuncType fn;

    static void apply(const Entt& e, const sol::object& obj);
};

struct RenderScale {
    f32 scale{1.0F};

    static void apply(const Entt& e, const sol::object& obj);
};

// TODO: add unique ID(name) component that maps on the UI base

struct RequestInternalBoundsUpdateFlag {};
struct RequestExternalBoundsUpdateFlag {};

}  // namespace mle::ui::comp
