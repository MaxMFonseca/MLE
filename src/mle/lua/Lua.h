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
#include "mle/common/Logger.h"
#include "mle/common/Result.h"
#include "mle/common/Types.h"

namespace mle::lua {
/// Initializes the Lua state.
Result init();
/// Shuts down the Lua state.
void shutdown();

/**
 * @brief Executes a Lua script file.
 * @param file Path to the Lua file to be executed.
 * @return The result of the script execution as a Lua object.
 */
sol::object scriptFile(const fs::path& file);

/**
 * @brief Loads a Lua module using `require`.
 * @param module_name Name of the Lua module.
 * @param engine Whether to load from engine-specific Lua module path.
 * @return The returned object from the required module.
 */
sol::object require(const std::string& module_name, bool engine = false);

/// Creates a new empty Lua table.
sol::table createTable();

/**
 * @brief Creates a named table in the global scope.
 * @param table Source Lua table to copy.
 * @param deep Whether to perform a deep copy.
 * @return A new Lua table copy.
 */
sol::table createTable(const std::string& name);

/**
 * @brief Creates a new Lua table based on an existing one.
 * @param table Source Lua table to copy.
 * @param deep Whether to perform a deep copy.
 * @return A new Lua table copy.
 */
sol::table createTable(const sol::table& table, bool deep = false);

namespace detail {
/// @brief Returns a reference to the internal Sol2 Lua state. Internal use only.
sol::state& getSol();
}  // namespace detail

/**
 * @brief Wraps a C++ object in a Lua object.
 * @tparam T Type of the object.
 * @param t The object to wrap.
 * @return A Lua object representing the C++ value.
 */
template <class T>
sol::object createObject(T&& t) {
    return sol::make_object(detail::getSol().lua_state(), std::forward<T>(t));
}

/**
 * @brief Registers a new Lua usertype.
 *
 * See [Sol2 usertype docs](https://sol2.readthedocs.io/en/latest/api/usertype.html) for details.
 *
 * @tparam T The C++ type to expose.
 * @tparam Args Constructor or member definitions to bind.
 * @param name Name of the usertype in Lua.
 * @param args Binding information (constructors, methods, etc.).
 * @return A reference to the created usertype object.
 */
template <class T, class... Args>
auto newUsertype(const std::string& name, Args&&... args) {
    MLE_T("Adding lua usertype: {}", name);
    return detail::getSol().new_usertype<T>(name, std::forward<Args>(args)...);
}

/**
 * @brief Gets an existing Lua usertype by name.
 * @tparam T The expected C++ type.
 * @param name Name of the usertype.
 * @return A reference to the usertype.
 */
template <class T>
auto getUserType(const std::string& name) {
    return detail::getSol().get<sol::usertype<T>>(name);
}

/**
 * @brief Unregisters a Lua usertype from the global state.
 * @tparam T The type of the usertype.
 * @param name Name of the usertype to remove.
 */
template <class T>
auto removeUserType(const std::string& name) {
    MLE_T("Removing lua usertype: {}", name);
    getUserType<T>(name).unregister();
}

/**
 * @brief Gets a Lua table by name.
 * @param name Name of the Lua table.
 * @return A reference to the Lua table.
 */
sol::table getTable(const std::string& name);

/**
 * @brief Registers a new enumeration in Lua.
 * @tparam Args Enum name followed by enum key/value pairs.
 * @param args Enum definition arguments.
 */
template <class... Args>
void newEnum(Args&&... args) {
    detail::getSol().new_enum(std::forward<Args>(args)...);
}

/**
 * @brief Adds a free function or callable to the Lua global namespace.
 * @tparam Args Function name followed by the function or callable object.
 * @param args Function binding arguments.
 */
template <class... Args>
void addFunction(Args&&... args) {
    detail::getSol().set_function(std::forward<Args>(args)...);
}

}  // namespace mle::lua
