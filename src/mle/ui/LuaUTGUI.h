#pragma once

#include "mle/lua/Lua.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"

namespace mle::lua {
inline void makeUTGUI(Lua& lua) {
    auto ut = lua.newUsertype<UI>("mle_UI", sol::no_constructor);
    ut["getElementById"] = [](UI& ui, const std::string& id) {
        auto e = ui.getE(id);
        if (!e) {
            MLE_E("Element with ID '{}' not found", id);
            return ui::Entt{ui, entt::null};
        }
        return e.value();
    };
}
}  // namespace mle::lua
