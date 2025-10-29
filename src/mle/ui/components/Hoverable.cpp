#include "Hoverable.h"

#include "mle/lua/Utils.h"
#include "mle/ui/Entt.h"
#include "mle/window/KeyUtils.h"
#include "mle/window/UserInputManager.h"
#include "sol/forward.hpp"

namespace mle::ui::comp {
void Hoverable::applyOnHover(const Entt& ew, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    if (obj.is<bool>()) {
        if (obj.as<bool>() == false) {
            ew.patchOrEmplace<Hoverable>([](Hoverable& c) { c.on_hover_out = nullptr; });
        } else {
            MLE_W("Function is a bool false, this does nothing.");
        }
        return;
    }

    if (!obj.is<sol::function>()) {
        MLE_E("Hoverable::applyOnHover expected a function, got {}.", obj.get_type());
        return;
    }
    ew.patchOrEmplace<Hoverable>([&](Hoverable& c) { c.on_hover = obj.as<sol::function>(); });
}

void Hoverable::applyOnHoverIn(const Entt& ew, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    if (obj.is<bool>()) {
        if (obj.as<bool>() == false) {
            ew.patchOrEmplace<Hoverable>([](Hoverable& c) { c.on_hover_out = nullptr; });
        } else {
            MLE_W("Function is a bool false, this does nothing.");
        }
        return;
    }

    if (!obj.is<sol::function>()) {
        MLE_E("Hoverable::applyOnHover expected a function, got {}.", obj.get_type());
        return;
    }
    ew.patchOrEmplace<Hoverable>([&](Hoverable& c) { c.on_hover_in = obj.as<sol::function>(); });
};

void Hoverable::applyOnHoverOut(const Entt& ew, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    if (obj.is<bool>()) {
        if (obj.as<bool>() == false) {
            ew.patchOrEmplace<Hoverable>([](Hoverable& c) { c.on_hover_out = nullptr; });
        } else {
            MLE_W("Function is a bool false, this does nothing.");
        }
        return;
    }

    if (!obj.is<sol::function>()) {
        MLE_E("Hoverable::applyOnHover expected a function, got {}.", obj.get_type());
        return;
    }

    ew.patchOrEmplace<Hoverable>([&](Hoverable& c) { c.on_hover_out = obj.as<sol::function>(); });
};

void Hoverable::applyOnKey(const Entt& ew, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    if (!obj.is<sol::table>()) {
        MLE_E("Hoverable::applyOnKey expected a table with {{key, fn/bool}} , got {}.", obj.get_type());
        return;
    }
    auto table = lua::as<sol::table>(obj);
    const sol::object key_r = table[1];
    const sol::object fn_r = table[2];

    if (!lua::valid<std::string>(key_r)) {
        MLE_E("Hoverable::applyOnKey expected a string as first argument, got {}.", key_r.get_type());
        return;
    }
    std::string key = key_r.as<std::string>();

    ew.patchOrEmplace<Hoverable>([&](Hoverable& c) { c.setKey(ew, key, fn_r); });
};

void Hoverable::applyOnKeys(const Entt& ew, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    if (!obj.is<sol::table>()) {
        MLE_E("Hoverable::applyOnKeys expected a table with many key:fn pairs, got {}.", obj.get_type());
        return;
    }

    ew.patchOrEmplace<Hoverable>([&](Hoverable& c) { c.setKeys(ew, lua::as<sol::table>(obj)); });
};

void Hoverable::removeKey(const Keybinding& kb) {
    auto kl_it = std::ranges::find_if(on_key, [&](const KeyListenerHnd& hnd) { return hnd->getKb() == kb; });
    if (kl_it != on_key.end()) {
        on_key.erase(kl_it);
    }
}

void Hoverable::setKey(const Entt& e, const Keybinding& kb, const sol::function& fn) {
    auto kl_it = std::ranges::find_if(on_key, [&](const KeyListenerHnd& hnd) { return hnd->getKb() == kb; });
    if (kl_it != on_key.end()) {
        (*kl_it)->setCallback([e, fn]() { fn(e); });
    } else {
        auto kl = std::make_unique<KeyListener>();
        kl->setKeybinding(kb);
        kl->setCallback([e, fn]() { fn(e); });
        on_key.emplace_back(std::move(kl));
    }
}

void Hoverable::setKey(const Entt& ew, const std::string& key, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    const auto kb_r = toKeybinding(key);
    if (!kb_r) {
        MLE_E("Invalid keybinding string '{}': {}", key, kb_r.error());
        return;
    }
    const auto& kb = *kb_r;

    if (obj.is<sol::function>()) {
        setKey(ew, kb, obj.as<sol::function>());
        return;
    }
    if (obj.is<bool>()) {
        if (obj.as<bool>()) {
            removeKey(kb);
        } else {
            MLE_W("OnKey function is a bool false, this does nothing.");
        }
        return;
    }
    MLE_E("Hoverable::setKey expected a function or bool, got {}.", obj.get_type());
}

void Hoverable::setKeys(const Entt& ew, const sol::table& table) {
    MLE_ASSERT(table.valid());
    for (const auto& [key, value] : table) {
        if (!lua::valid<std::string>(key)) {
            MLE_E("Hoverable::setKeys expected string keys, got {}.", key.get_type());
            continue;
        }
        setKey(ew, key.as<std::string>(), value);
    }
};

void Hoverable::onHoverOut(const Entt& ew) {
    for (auto& kl : on_key) {
        kl->unlisten();
    }
    if (on_hover_out) {
        on_hover_out(ew);
    }
}

void Hoverable::onHover(const Entt& ew) {
    if (on_hover) {
        on_hover(ew);
    }
};

void Hoverable::onHoverIn(const Entt& ew) {
    for (auto& kl : on_key) {
        kl->listen();
    }
    if (on_hover_in) {
        on_hover_in(ew);
    }
};

sol::object Hovered::get(const Entt& ew) {
    MLE_ASSERT(ew.has<Hovered>());

    return ew.ui().getLua().createObject(ew.get<Hovered>());
}

void Hovered::makeLuaUsertype(Lua& lua) {
    lua.newUsertype<Hovered>("uiHovered", "state", &Hovered::state, "sw", &Hovered::sw, "pos_self", &Hovered::pos_self, "pos_self_norm",
                             &Hovered::pos_self_norm);
}
}  // namespace mle::ui::comp
