#include "LuaKeyHandlers.h"

#include "mle/lua/Types.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Image.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/Base.h"
#include "mle/ui/element/ListLayout.h"
#include "sol/forward.hpp"

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
    addLuaKeyHandler("background", background);
    addLuaKeyHandler("list", ListLayout::lkhList);
    addLuaKeyHandler("root_image", comp::RootImage::lkh);
}
}  // namespace mle::ui::element
