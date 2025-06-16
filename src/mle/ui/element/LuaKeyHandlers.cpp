#include "LuaKeyHandlers.h"

#include <sol/forward.hpp>

#include "mle/lua/Types.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Image.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/Base.h"
#include "mle/ui/element/Container.h"
#include "mle/ui/element/ListLayout.h"

namespace mle::ui::element {
namespace {
void name(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    if (obj.is<std::string>()) {
        reg.emplace_or_replace<comp::Name>(self, obj.as<std::string>());
    } else {
        MLE_UNREACHABLE_LOG("Unexpected obj type for Name: {}", obj.get_type());
    }
}

void size(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetSize>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetSize>(self);
    }

    if (obj.is<f32>()) {
        comp->x.val = comp->y.val = obj.as<f32>();
    } else {
        MLE_ASSERT(obj.is<sol::table>());
        auto table = obj.as<sol::table>();
        auto x_r = table[1];
        if (x_r.valid()) {
            comp->x.fromLua(x_r);
        } else {
            x_r = table["x"];
            if (x_r.valid()) {
                comp->x.fromLua(x_r);
            }
        }
        auto y_r = table[2];
        if (x_r.valid()) {
            comp->y.fromLua(y_r);
        } else {
            y_r = table["y"];
            if (y_r.valid()) {
                comp->y.fromLua(y_r);
            }
        }
    }

    comp::Container::notifyChildChangedBounds(self);
}

void sizeX(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetSize>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetSize>(self);
    }

    comp::Container::notifyChildChangedBounds(self);
}

void sizeY(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetSize>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetSize>(self);
    }

    comp::Container::notifyChildChangedBounds(self);
}

void padding(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();
    auto* target_padding_comp = reg.try_get<comp::TargetPadding>(self);
    if (!target_padding_comp) {
        target_padding_comp = &reg.emplace<comp::TargetPadding>(self);
    }

    if (obj.is<f32>()) {
        f32 val = obj.as<f32>();
        target_padding_comp->t.val = target_padding_comp->b.val = target_padding_comp->l.val = target_padding_comp->r.val = val;
    } else {
        MLE_ASSERT(obj.is<sol::table>());
        auto table = obj.as<sol::table>();

        auto t_r = lua::getKeyOrIdx(table, "t", 1);
        auto b_r = lua::getKeyOrIdx(table, "b", 2);
        auto l_r = lua::getKeyOrIdx(table, "l", 3);
        auto r_r = lua::getKeyOrIdx(table, "r", 4);

        if (t_r) {
            target_padding_comp->t.fromLua(*t_r);
        }
        if (b_r) {
            target_padding_comp->b.fromLua(*b_r);
        }
        if (l_r) {
            target_padding_comp->l.fromLua(*l_r);
        }
        if (r_r) {
            target_padding_comp->r.fromLua(*r_r);
        }
    }
}

void margin(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();
    auto* target_margin_comp = reg.try_get<comp::TargetMargin>(self);

    if (!target_margin_comp) {
        target_margin_comp = &reg.emplace<comp::TargetMargin>(self);
    }

    if (obj.is<f32>()) {
        f32 val = obj.as<f32>();
        target_margin_comp->t.val = target_margin_comp->b.val = target_margin_comp->l.val = target_margin_comp->r.val = val;
    } else if (obj.is<std::string>()) {
        target_margin_comp->r.fromString(obj.as<std::string>());
        target_margin_comp->t = target_margin_comp->b = target_margin_comp->l = target_margin_comp->r;
    } else {
        MLE_ASSERT(obj.is<sol::table>());
        auto table = obj.as<sol::table>();

        auto t_r = lua::getKeyOrIdx(table, "t", 1);
        auto b_r = lua::getKeyOrIdx(table, "b", 2);
        auto l_r = lua::getKeyOrIdx(table, "l", 3);
        auto r_r = lua::getKeyOrIdx(table, "r", 4);

        if (t_r) {
            target_margin_comp->t.fromLua(*t_r);
        }
        if (b_r) {
            target_margin_comp->b.fromLua(*b_r);
        }
        if (l_r) {
            target_margin_comp->l.fromLua(*l_r);
        }
        if (r_r) {
            target_margin_comp->r.fromLua(*r_r);
        }
    }
}

void background(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    reg.emplace_or_replace<comp::Background>(self, Color::fromLua(obj));
}

auto& getMap() {
    static std::unordered_map<std::string, LuaKeyHandlerFn> lua_keys;
    return lua_keys;
}
}  // namespace

void addLuaKeyHandler(const std::string& key, LuaKeyHandlerFn fn) {
    MLE_D("Adding Lua key handler for '{}', fn: {}", key, rAs<void*>(fn));
    MLE_ASSERT(!getMap().contains(key));
    getMap()[key] = fn;
}

std::optional<LuaKeyHandlerFn> getLuaKeyHandlerFn(const std::string& key) {
    auto it = getMap().find(key);
    if (it == getMap().end()) {
        return std::nullopt;
    }
    return it->second;
}

void addEngineLuaKeyHandlers() {
    addLuaKeyHandler("name", name);
    addLuaKeyHandler("size", size);
    addLuaKeyHandler("size_x", sizeX);
    addLuaKeyHandler("size_y", sizeY);
    addLuaKeyHandler("padding", padding);
    addLuaKeyHandler("margin", margin);
    addLuaKeyHandler("background", background);
    addLuaKeyHandler("list", ListLayout::lkhList);
    addLuaKeyHandler("root_image", comp::RootImage::lkh);
}
}  // namespace mle::ui::element
