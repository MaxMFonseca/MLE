#pragma once

#include "mle/lua/Types.h"
#include "mle/ui/Types.h"
#include "mle/window/Types.h"

namespace mle::ui::comp {
struct Hoverable {
    HoverFn on_hover;
    HoverFn on_hover_in;
    HoverFn on_hover_out;
    std::vector<KeyListenerHnd> on_key;

    void setKeys(const Entt& ew, const sol::table& table);
    void setKey(const Entt& ew, const std::string& key, const sol::object& obj);
    void setKey(const Entt& ew, const Keybinding& kb, const sol::function& fn);
    void setKey(const Entt& ew, const Keybinding& kb, std::move_only_function<void(const Entt&)> fn);
    void removeKey(const Keybinding& kb);

    void onHoverIn(const Entt& ew);
    void onHover(const Entt& ew);
    void onHoverOut(const Entt& ew);

    static void applyOnHover(const Entt& ew, const sol::object& obj);
    static void applyOnHoverIn(const Entt& ew, const sol::object& obj);
    static void applyOnHoverOut(const Entt& ew, const sol::object& obj);
    static void applyOnKey(const Entt& ew, const sol::object& obj);
    static void applyOnKeys(const Entt& ew, const sol::object& obj);
};

struct Hovered {
    HoverState state{};
    Stopwatch sw{};
    vec2f pos_self{};
    vec2f pos_self_norm{};

    static sol::object get(const Entt& ew, const sol::object& params = {});

    static void makeLuaUsertype(Lua& lua);
};

}  // namespace mle::ui::comp
