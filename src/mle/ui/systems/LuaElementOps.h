#pragma once

#include "../Types.h"
#include "mle/lua/Types.h"
#include "sol/forward.hpp"

namespace mle::ui::system {
class LuaElementOps final {
  public:
    using KeyHandlerFn = void (*)(const Entt& e, const sol::object& obj);

  public:
    explicit LuaElementOps(UI& ui);

    void applyTable(entt::entity e, const sol::table& table);
    void applyObj(entt::entity e, const std::string& key, const sol::object& obj);
    void addKeyHandler(const std::string& key, KeyHandlerFn fn);

  private:
    UI& ui_;

    std::unordered_map<std::string, KeyHandlerFn> key_handlers_;
};
}  // namespace mle::ui::system
