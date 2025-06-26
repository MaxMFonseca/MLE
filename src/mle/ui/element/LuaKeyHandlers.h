#pragma once

#include "Base.h"
#include "mle/ui/Types.h"
#include "sol/forward.hpp"

namespace mle::ui::element {
using LuaKeyHandlerFn = void (*)(entt::entity e, const sol::object& obj);
void addLuaKeyHandler(const std::string& key, LuaKeyHandlerFn fn);
std::optional<LuaKeyHandlerFn> getLuaKeyHandlerFn(const std::string& key);
void addEngineLuaKeyHandlers();
void applyEntityTable(entt::entity e, const sol::table& table);
}  // namespace mle::ui::element
