#pragma once

#include "Base.h"

namespace mle::ui::element {
using LuaKeyHandlerFn = void (*)(entt::entity e, const sol::object& obj);
void addLuaKeyHandler(const std::string& key, LuaKeyHandlerFn fn);
std::optional<LuaKeyHandlerFn> getLuaKeyHandlerFn(const std::string& key);
void addEngineLuaKeyHandlers();
}  // namespace mle::ui::element
