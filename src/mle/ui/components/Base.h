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
    Color lt = Color::ZERO;
    Color rt = Color::ZERO;
    Color lb = Color::ZERO;
    Color rb = Color::ZERO;

    void set(const sol::object& obj);

    static void apply(const Entt& e, const sol::object& obj);
};

struct Layer {
    int layer;

    void set(int l) { layer = l; }

    static void apply(const Entt& e, const sol::object& obj);
};

struct Table {
    sol::table o;

    static void apply(const Entt& e, const sol::object& obj);

    static sol::object get(const Entt& e, const sol::object& params);
};

struct Functions {
    std::map<std::string, sol::function> fns;

    [[nodiscard]] sol::function get(const std::string& name) const;

    static void apply(const Entt& e, const sol::object& obj);

    static sol::object get(const Entt& e, const sol::object& params);
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

struct RequestInternalBoundsUpdateFlag {};
struct RequestExternalBoundsUpdateFlag {};
struct CursorDragFlag {};
struct ForceFitFlag {
    static void apply(const Entt& e, const sol::object& obj);
};

struct DisabledFlag {
    static void applyEnabled(const Entt& e, const sol::object& obj);
    static void applyDisabled(const Entt& e, const sol::object& obj);
};

}  // namespace mle::ui::comp
