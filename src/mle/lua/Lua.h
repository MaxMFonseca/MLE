/**
 * @file
 * @brief Lua scripting system integration for the engine.
 *
 * Provides an interface to a single Lua context using Sol2. This system is currently not
 * multithreaded and supports only one global Lua state. It offers helpers for creating tables,
 * objects, user types, enumerations, and functions for script interaction.
 *
 * Future versions may support multiple Lua states to enable parallel script execution without
 * synchronization overhead.
 */

#pragma once

#include <sol/forward.hpp>
#include <sol/state.hpp>

#include "Types.h"
#include "mle/core/Assert.h"
#include "mle/core/Logger.h"
#include "mle/utils/Utils.h"

namespace mle {
class Lua {
  public:
    MLE_NO_COPY_MOVE(Lua)

    Lua() = default;
    ~Lua() = default;

    void init();

    sol::object require(const std::string& module_name);
    Expected<sol::object> tryRequire(const std::string& module_name);

    sol::table createTable();
    sol::table createTable(const std::string& name);
    sol::table createTable(const sol::table& table, bool deep = false);
    sol::table mergeTablesNew(const sol::table& a, const sol::table& b);
    void mergeTables(sol::table& dst, const sol::table& src);

    sol::table getTable(const std::string& name);

  private:
    sol::state& getSol() { return sol_; };

  public:
    template <class T>
    sol::object createObject(T&& t) {
        return sol::make_object(sol_, std::forward<T>(t));
    }

    template <class T>
    void setGlobal(const std::string& name, T&& t) {
        MLE_I("Creating global Lua object: {}", name);
        sol_.set(name, createObject(std::forward<T>(t)));
    }

    template <class T>
    auto getGlobal(const std::string& name) {
        MLE_I("Getting global Lua object: {}", name);
        return sol_.get<T>(name);
    }

    template <class T, class... Args>
    auto newUsertype(const std::string& name, Args&&... args) {
        MLE_I("Adding lua usertype: {}", name);
        MLE_ASSERT_LOG(!name.empty(), "Lua usertype name must not be empty");
        MLE_ASSERT_LOG(!sol_.get<sol::object>(name).valid(), "An object with the name '{}' already exists!", name);
        return sol_.new_usertype<T>(name, std::forward<Args>(args)...);
    }

    template <class T>
    auto getUsertype(const std::string& name) {
        return sol_.get<sol::usertype<T>>(name);
    }

    template <class T>
    auto removeUsertype(const std::string& name) {
        MLE_I("Removing lua usertype: {}", name);
        getUsertype<T>(name).unregister();
    }

    template <class... Args>
    void newEnum(Args&&... args) {
        MLE_I("Adding lua enum: {}", std::get<0>(std::forward_as_tuple(args...)));
        sol_.new_enum(std::forward<Args>(args)...);
    }

    template <class... Args>
    void setFunction(Args&&... args) {
        MLE_I("Adding lua function: {}", std::get<0>(std::forward_as_tuple(args...)));
        sol_.set_function(std::forward<Args>(args)...);
    }

  private:
    sol::state sol_;
};
}  // namespace mle
