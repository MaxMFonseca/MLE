#include "LuaKeyHandlers.h"

#include "mle/lua/Types.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"

namespace mle::ui::element {
namespace {
void name(entt::entity e, const sol::object& obj) {
    auto& reg = getRegistry();

    if (obj.is<std::string>()) {
        reg.emplace_or_replace<Name>(e, obj.as<std::string>());
    }

    MLE_UNREACHABLE_LOG("Unexpected obj type for Name: {}", obj.get_type());
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
}
}  // namespace mle::ui::element
