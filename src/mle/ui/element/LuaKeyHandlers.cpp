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
#include "mle/ui/element/Shader.h"
#include "mle/ui/element/Text.h"
#include "mle/ui/element/Types.h"

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

void shader(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* renderable = reg.try_get<comp::Renderable>(self);
    if (!renderable) {
        renderable = &reg.emplace<comp::Renderable>(self, std::make_unique<Shader>());
    }

    renderable->apply(self, obj);
}

template <typename T, bool Click = false>
void collidable(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<T>(self);
    if (!comp) {
        comp = &reg.emplace<T>(self);
        comp::Collidable::add(self);
    }
    if constexpr (Click) {
        reg.try_get<comp::Collidable>(self)->clicable = true;
    }

    comp->fn = lua::as<sol::function>(obj);
}

void table(entt::entity self, const sol::object& obj) {
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::Table>(self);
    if (!comp) {
        comp = &reg.emplace<comp::Table>(self);
    }

    comp->apply(self, lua::as<sol::table>(obj));
}

void onInit(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::OnInit>(self);
    if (!comp) {
        comp = &reg.emplace<comp::OnInit>(self);
    }

    comp->fn = lua::as<sol::function>(obj);
}

void onUpdate(entt::entity self, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    auto& reg = getRegistry();

    auto* comp = reg.try_get<comp::OnUpdate>(self);
    if (!comp) {
        comp = &reg.emplace<comp::OnUpdate>(self);
    }

    comp->fn = lua::as<sol::function>(obj);
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

void EntityWrapper::dispatchEvent(const std::string& event_name) {
    ui::dispatchEvent(event_name);
}

#define MLE_ASSERT_HAS(T) MLE_ASSERT_LOG(getRegistry().all_of<T>(o), "Entity does not have component {}", #T)

sol::table EntityWrapper::getTable() const {
    MLE_ASSERT_HAS(comp::Table);
    return getRegistry().get<comp::Table>(o).v;
}

void addEngineLuaKeyHandlers() {
    auto ut = lua::newUsertype<EntityWrapper>("entt");
    ut["apply"] = &EntityWrapper::apply;
    ut["getTable"] = &EntityWrapper::getTable;
    ut["dispatchEvent"] = &EntityWrapper::dispatchEvent;

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
    addLuaKeyHandler("shader", shader);
    addLuaKeyHandler("table", table);
    addLuaKeyHandler("on_init", onInit);
    addLuaKeyHandler("on_update", onUpdate);
    addLuaKeyHandler("on_hover", collidable<comp::OnHover>);
    addLuaKeyHandler("on_hover_enter", collidable<comp::OnHoverEnter>);
    addLuaKeyHandler("on_hover_leave", collidable<comp::OnHoverLeave>);
    addLuaKeyHandler("on_click", collidable<comp::OnLeftPressed, true>);
    addLuaKeyHandler("on_left_pressed", collidable<comp::OnLeftPressed, true>);
    addLuaKeyHandler("on_left_released", collidable<comp::OnLeftReleased, true>);
    addLuaKeyHandler("on_left_down", collidable<comp::OnLeftDown, true>);
}
}  // namespace mle::ui::element
