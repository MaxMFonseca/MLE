#pragma once

#include "Base.h"
#include "mle/ui/Types.h"
#include "sol/forward.hpp"

namespace mle::ui::element {
struct EntityWrapper {
    entt::entity o;

    void apply(const std::string& key, const sol::object& obj) const;
    [[nodiscard]] sol::table getTable() const;
};
using EWrap = EntityWrapper;

using LuaKeyHandlerFn = void (*)(entt::entity e, const sol::object& obj);
void addLuaKeyHandler(const std::string& key, LuaKeyHandlerFn fn);
std::optional<LuaKeyHandlerFn> getLuaKeyHandlerFn(const std::string& key);
void addEngineLuaKeyHandlers();
void applyEntityTable(entt::entity e, const sol::table& table);
}  // namespace mle::ui::element
