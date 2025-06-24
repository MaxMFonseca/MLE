#include "LuaKeyHandlers.h"

#include <sol/forward.hpp>

#include "Sprite.h"
#include "mle/common/Assert.h"
#include "mle/lua/Lua.h"
#include "mle/lua/Types.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Image.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/Base.h"
#include "mle/ui/element/Bounds.h"
#include "mle/ui/element/Collidable.h"
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

void rootImage(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::RootImage>(self);
    if (!comp) {
        comp = &reg.emplace<comp::RootImage>(self);
    }

    comp->apply(self, obj);
}

void pos(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetPosition>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetPosition>(self);
    }

    comp->apply(self, obj);
}

void posX(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetPosition>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetPosition>(self);
    }

    comp->applyX(self, obj);
}

void posY(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetPosition>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetPosition>(self);
    }

    comp->applyY(self, obj);
}

void size(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetSize>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetSize>(self);
    }

    comp->apply(self, obj);
}

void sizeX(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetSize>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetSize>(self);
    }

    comp->applyX(self, obj);
}

void sizeY(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetSize>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetSize>(self);
    }

    comp->applyY(self, obj);
}

void padding(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::Padding>(self);
    if (!comp) {
        comp = &reg.emplace<comp::Padding>(self);
    }

    comp->apply(self, obj);
}

void margin(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetMargin>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetMargin>(self);
    }

    comp->apply(self, obj);
}

void origin(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetOrigin>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetOrigin>(self);
    }

    comp->apply(self, obj);
}

void rel(entt::entity self, const sol::object& obj) {
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetRelations>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetRelations>(self);
    }

    comp->apply(self, obj);
}

void aspectRatio(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::TargetAspectRatio>(self);
    if (!comp) {
        comp = &reg.emplace<comp::TargetAspectRatio>(self);
    }

    comp->apply(self, obj);
}

void background(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto color = Color::fromLua(obj);
    MLE_VC(color);
    reg.emplace_or_replace<comp::Background>(self, color);
}

void blur(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    reg.emplace_or_replace<comp::Blur>(self);
}

void container(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::Container>(self);
    if (!comp) {
        comp = &reg.emplace<comp::Container>(self);
    }

    auto* renderable = reg.try_get<comp::Renderable>(self);
    if (!renderable) {
        reg.emplace<comp::Renderable>(self);
    }

    comp->apply(self, obj);
}

void sprite(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* renderable = reg.try_get<comp::Renderable>(self);
    if (!renderable) {
        renderable = &reg.emplace<comp::Renderable>(self, std::make_unique<Sprite>());
    }

    renderable->apply(self, obj);
}

void text(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* renderable = reg.try_get<comp::Renderable>(self);
    if (!renderable) {
        renderable = &reg.emplace<comp::Renderable>(self, std::make_unique<Text>());
    }

    renderable->apply(self, obj);
}

void onHover(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::OnHover>(self);
    if (!comp) {
        comp = &reg.emplace<comp::OnHover>(self);
        comp::Collidable::add(self);
    }

    comp->fn = [fn = lua::as<sol::function>(obj)](entt::entity self) { fn(EWrap(self)); };
}

void onHoverEnter(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::OnHoverEnter>(self);
    if (!comp) {
        comp = &reg.emplace<comp::OnHoverEnter>(self);
        comp::Collidable::add(self);
    }

    comp->fn = [fn = lua::as<sol::function>(obj)](entt::entity self) { fn(EWrap(self)); };
}

void onHoverLeave(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::OnHoverLeave>(self);
    if (!comp) {
        comp = &reg.emplace<comp::OnHoverLeave>(self);
        comp::Collidable::add(self);
    }

    comp->fn = [fn = lua::as<sol::function>(obj)](entt::entity self) { fn(EWrap(self)); };
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

void applyEntityTable(entt::entity e, const sol::table& table) {
    MLE_ASSERT(table.valid());

    for (const auto& [key, value] : table) {
        std::string key_str;
        if (!lua::tryAs(key, key_str)) {
            continue;
        }

        EntityWrapper{e}.apply(key_str, value);
    }
}

void EntityWrapper::apply(const std::string& key, const sol::object& obj) const {
    MLE_ASSERT(obj.valid());
    auto fn_r = getLuaKeyHandlerFn(key);
    if (!fn_r) {
        MLE_W("No Lua key handler for '{}'", key);
        return;
    }
    fn_r.value()(o, obj);
}

void addEngineLuaKeyHandlers() {
    auto ut = lua::newUsertype<EntityWrapper>("entt");
    ut["apply"] = &EntityWrapper::apply;

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
    addLuaKeyHandler("container", container);
    addLuaKeyHandler("c", container);
    addLuaKeyHandler("root_image", rootImage);
    addLuaKeyHandler("sprite", sprite);
    addLuaKeyHandler("text", text);
    addLuaKeyHandler("on_hover", onHover);
    addLuaKeyHandler("on_hover_enter", onHoverEnter);
    addLuaKeyHandler("on_hover_leave", onHoverLeave);
}
}  // namespace mle::ui::element
