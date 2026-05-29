#pragma once

#include "mle/lua/Lua.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"

namespace mle::lua {
inline void makeUTGUI(Lua& lua) {
    auto entt_table = lua.createTable("entt");
    entt_table["null"] = static_cast<entt::entity>(entt::null);

    auto ut = lua.newUsertype<UI>("mle_UI", sol::no_constructor);
    ut["getElementById"] = [](UI& ui, const std::string& id) {
        auto e = ui.getE(id);
        if (!e) {
            MLE_E("Element with ID '{}' not found", id);
            return ui::Entt{ui, entt::null};
        }
        return e.value();
    };
    ut["hitTest"] = [](UI& ui, f32 x, f32 y, sol::optional<ui::Entt> ignore_ew) {
        entt::entity ignore = ignore_ew ? ignore_ew->e() : entt::null;
        entt::entity hit = ui.hoverSystem().hitTest(vec2f{x, y}, ignore);
        if (hit == entt::null) {
            return ui::Entt{ui, entt::null};
        }
        return ui::Entt{ui, hit};
    };
    ut["logAllBounds"] = [](UI& ui) { ui.logAllBounds(); };
}
}  // namespace mle::lua
