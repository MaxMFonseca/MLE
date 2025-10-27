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

    void setKeys(const sol::table& table);
    void setKey(const std::string& key, const sol::object& obj);
    void setKey(const Keybinding& kb, const sol::function& fn);
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
    HoverState state;
    Stopwatch sw;
};

}  // namespace mle::ui::comp
