#pragma once

#include "../Types.h"
#include "mle/lua/Types.h"
#include "sol/forward.hpp"

namespace mle::ui::system {
struct LuaElementOps final {
    using ApplyKeyFn = void (*)(const Entt& e, const sol::object& obj);
    using GetKeyFn = sol::object (*)(const Entt& e, const sol::object& params);

    static void init();

    static void applyTable(const Entt& ew, const sol::table& table);
    static void applyObj(const Entt& ew, const std::string& key, const sol::object& obj);
    static sol::object getKey(const Entt& ew, const std::string& key, const sol::object& params = {});

    static void addGetKeyHandler(const std::string& key, GetKeyFn fn);
    static void addApplyKeyHandler(const std::string& key, ApplyKeyFn fn);

    static void addBuiltingApply();
    static void addBuiltingGetters();

    static std::unordered_map<std::string, ApplyKeyFn> apply_key_handlers_;
    static std::unordered_map<std::string, GetKeyFn> get_key_handlers_;
};
}  // namespace mle::ui::system
