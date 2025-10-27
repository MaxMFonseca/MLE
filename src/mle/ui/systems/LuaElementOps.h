#pragma once

#include "../Types.h"
#include "mle/lua/Types.h"
#include "sol/forward.hpp"

namespace mle::ui::system {
class LuaElementOps final {
  public:
    using ApplyKeyFn = void (*)(const Entt& e, const sol::object& obj);
    using GetKeyFn = sol::object (*)(const Entt& e);

  public:
    explicit LuaElementOps(UI& ui);

    void applyTable(entt::entity e, const sol::table& table);
    void applyObj(entt::entity e, const std::string& key, const sol::object& obj);
    sol::object getKey(entt::entity e, const std::string& key);

    void addApplyKeyHandler(const std::string& key, ApplyKeyFn fn);
    void addGetKeyHandler(const std::string& key, GetKeyFn fn);

    void addBuiltingApply();
    void addBuiltingGetters();

  private:
    UI& ui_;

    std::unordered_map<std::string, ApplyKeyFn> apply_key_handlers_;
    std::unordered_map<std::string, GetKeyFn> get_key_handlers_;
};
}  // namespace mle::ui::system
