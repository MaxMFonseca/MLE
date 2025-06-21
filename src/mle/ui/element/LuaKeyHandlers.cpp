#include "LuaKeyHandlers.h"

#include <sol/forward.hpp>

#include "Sprite.h"
#include "mle/lua/Types.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Image.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/Base.h"
#include "mle/ui/element/Container.h"
#include "mle/ui/element/Text.h"

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

void pos(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetPosition>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetPosition>(self);
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

void posX(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetPosition>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetPosition>(self);
    }

    comp->x.fromLua(obj);

    comp::Container::notifyChildChangedBounds(self);
}

void posY(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetPosition>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetPosition>(self);
    }

    comp->y.fromLua(obj);

    comp::Container::notifyChildChangedBounds(self);
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

    comp->x.fromLua(obj);

    comp::Container::notifyChildChangedBounds(self);
}

void sizeY(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetSize>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetSize>(self);
    }

    comp->y.fromLua(obj);

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

    // FIXME: Must notify somehow that padding has changed
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

    comp::Container::notifyChildChangedBounds(self);
}

namespace {
f32 charToOrigin(char c) {
    switch (c) {
        case 'l':
        case 't':
            return 0.0F;
        case 'r':
        case 'b':
            return 1.0F;
        case 'x':
        case 'c':
            return 0.5F;
        default:
            MLE_W("Unexpected char in origin string: '{}'", c);
            return 0.0F;
    }
}

vec2f stringToOrigin(const std::string& s) {
    vec2f ret{};
    ret.x = charToOrigin(s[0]);
    if (s.size() > 1) {
        ret.y = charToOrigin(s[1]);
    } else {
        ret.y = ret.x;
    }
    return ret;
}
}  // namespace

void origin(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::Origin>(self);
    if (!comp) {
        comp = &reg.emplace<comp::Origin>(self);
    }

    if (obj.is<std::string>()) {
        comp->origin = stringToOrigin(obj.as<std::string>());
    } else {
        MLE_ASSERT(obj.is<sol::table>());

        if (const auto x_r = lua::getKeyOrIdx(obj.as<sol::table>(), "x", 1); x_r) {
            if (x_r->is<std::string>()) {
                comp->origin.x = charToOrigin(x_r->as<std::string>()[0]);
            } else if (x_r->is<f32>()) {
                comp->origin.x = x_r->as<f32>();
            } else {
                MLE_UNREACHABLE_LOG("Unexpected x type for Origin: {}", x_r->get_type());
            }
        }

        if (const auto y_r = lua::getKeyOrIdx(obj.as<sol::table>(), "y", 2); y_r) {
            if (y_r->is<std::string>()) {
                comp->origin.y = charToOrigin(y_r->as<std::string>()[0]);
            } else if (y_r->is<f32>()) {
                comp->origin.y = y_r->as<f32>();
            } else {
                MLE_UNREACHABLE_LOG("Unexpected y type for Origin: {}", y_r->get_type());
            }
        }
    }
}

void rel(entt::entity self, const sol::object& obj) {
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetRelations>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetRelations>(self);
    }

    auto& parent = comp::Parent::container(self);

    auto table = lua::as<sol::table>(obj);
    for (const auto& [_, v] : table) {
        auto str = lua::as<std::string>(v);
        auto ss = split(str, ':');

        if (ss.size() != 3) {
            MLE_W("Invalid relation format: '{}'. Expected format is 'type:target:offset'.", str);
            continue;
        }

        comp::TargetRelations::Dep dep;
        comp::TargetRelations::Dep dep2;
        bool duo_dep = false;

        if (ss[0] == "pos_x") {
            dep.type = comp::TargetRelations::Dep::Type::POS_X;
        } else if (ss[0] == "pos_y") {
            dep.type = comp::TargetRelations::Dep::Type::POS_Y;
        } else if (ss[0] == "size_x") {
            dep.type = comp::TargetRelations::Dep::Type::SIZE_X;
        } else if (ss[0] == "size_y") {
            dep.type = comp::TargetRelations::Dep::Type::SIZE_Y;
        } else if (ss[0] == "pos") {
            dep.type = comp::TargetRelations::Dep::Type::POS_X;
            dep2.type = comp::TargetRelations::Dep::Type::POS_Y;
            duo_dep = true;
        } else if (ss[0] == "size") {
            dep.type = comp::TargetRelations::Dep::Type::SIZE_X;
            dep2.type = comp::TargetRelations::Dep::Type::SIZE_Y;
            duo_dep = true;
        }

        dep2.e = dep.e = parent.getChild(ss[1]);
        if (dep.e == entt::null) {
            MLE_W("Invalid relation. Target {} not found on parent.", ss[1]);
            continue;
        }

        if (std::isdigit(ss[2][0]) || ss[2][0] == '.') {
            dep2.val = dep.val = std::stof(ss[2]);
        } else {
            auto v = stringToOrigin(ss[2]);
            dep.val = v.x;
            dep2.val = v.y;
        }

        comp->v.emplace_back(dep);
        if (duo_dep) {
            comp->v.emplace_back(dep2);
        }
    }
}

void aspectRatio(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetAspectRatio>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetAspectRatio>(self);
    }

    if (obj.is<f32>()) {
        comp->v = obj.as<f32>();
    } else {
        MLE_UNREACHABLE_LOG("Unexpected obj type for TargetAspectRatio: {}", obj.get_type());
    }

    comp::Container::notifyChildChangedBounds(self);
}

void background(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    reg.emplace_or_replace<comp::Background>(self, Color::fromLua(obj));
}

void blur(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    reg.emplace_or_replace<comp::Blur>(self);
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
    addLuaKeyHandler("pos", pos);
    addLuaKeyHandler("pos_x", posX);
    addLuaKeyHandler("pos_y", posY);
    addLuaKeyHandler("padding", padding);
    addLuaKeyHandler("margin", margin);
    addLuaKeyHandler("origin", origin);
    addLuaKeyHandler("aspect_ratio", aspectRatio);
    addLuaKeyHandler("background", background);
    addLuaKeyHandler("blur", blur);
    addLuaKeyHandler("rel", rel);
    addLuaKeyHandler("children", comp::Container::lkh);
    addLuaKeyHandler("container", comp::Container::lkh);
    addLuaKeyHandler("c", comp::Container::lkh);
    addLuaKeyHandler("root_image", comp::RootImage::lkh);
    addLuaKeyHandler("sprite", comp::Sprite::lkh);
    addLuaKeyHandler("text", comp::Text::lkh);
}
}  // namespace mle::ui::element
