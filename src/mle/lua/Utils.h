/**
 * @file
 * @brief Lua utility functions.
 *
 * Most functions are unsafe in release mode, so ensure the Lua objects are of the expected type.
 */

// TODO: Maybe add Expected<T> return variants for user code later.

#pragma once

#include <cstddef>
#include <sol/sol.hpp>

#include "Types.h"
#include "mle/common/Assert.h"
#include "mle/common/Logger.h"
#include "mle/common/math/Types.h"
#include "mle/common/math/Types2D.h"
#include "mle/core/Core.h"
#include "sol/forward.hpp"

namespace mle::lua {
/**
 * @brief Checks if the object is valid and matches the given type.
 *
 * @tparam T Expected C++ type.
 * @param obj The Lua object to test.
 * @return True if the object is valid and can be cast to T.
 */
template <typename T>
inline bool valid(const sol::object& obj) {
    return obj.valid() && obj.is<T>();
}

template <typename T>
inline void assertIs(const sol::object& obj) {
    if (!valid<T>(obj)) {
        core::unrecoverable("Object is not valid or not of type {}", obj.get_type());
    }
}

/**
 * @brief Validates a list of Lua objects against the expected type.
 *
 * @tparam T Expected C++ type.
 * @param first First object to validate.
 * @param rest Remaining objects.
 * @return True if all objects are valid and of type T.
 */
template <typename T, typename... Args>
inline bool validAll(const sol::object& first, const Args&... rest) {
    return valid<T>(first) && (valid<T>(rest) && ...);
}

/**
 * @brief Gets a Lua object as a specific type T.
 *
 * This function asserts that the object is valid and can be converted to T.
 * It is unsafe in release mode if the object is not of the expected type.
 *
 * @tparam T Expected C++ type.
 * @param obj The Lua object to convert.
 * @return The object converted to type T.
 */
template <typename T>
inline T as(const sol::object& obj) {
    MLE_ASSERT_LOG(valid<T>(obj), "Object is not valid or not of type {}", obj.get_type());
    return obj.as<T>();
}

/**
 * @brief Try to get a lua object as a specific type T.
 *
 * @tparam T Expected C++ type.
 * @param obj The Lua object to convert.
 * @param out Output reference to store the converted value.
 * @return True if the object was successfully converted, false otherwise.
 */
template <typename T>
inline bool tryAs(const sol::object& obj, T& out) {
    if (!valid<T>(obj)) {
        return false;
    }
    out = obj.as<T>();
    return true;
}

/**
 * @brief Extract sequential values from a Lua table(list) by index.
 *
 * This function asserts on debug mode that the table has enough elements to fill all output references
 * and that they are all of the expected type T.
 * It is unsafe in release mode if the table does not have enough elements or if the types do not match.
 *
 * @tparam T Expected value type.
 * @param table Lua table object.
 * @param out Output references (must all be of type T).
 * @return True if all values were successfully extracted.
 */
template <typename T, typename... Args>
inline void getList(const sol::table& table, Args&... out) {
    static_assert((std::is_same_v<T, std::remove_cv_t<std::remove_reference_t<Args>>> && ...), "All output parameters must be of type T");
    int index = 1;
    auto one = [&](auto& ref) { ref = as<T>(table[index++]); };
    (one(out), ...);
}

/**
 * @brief Try to extract sequential values from a Lua table(list) by index.
 *
 * No values are written to `out` if the table does not have enough elements or if the types do not match.
 *
 * @tparam T Expected value type.
 * @param table Lua table object.
 * @param out Output references (must all be of type T).
 * @return True if all values were successfully extracted, false otherwise.
 */
template <typename T, typename... Args>
inline bool tryGetList(const sol::table& table, Args&... out) {
    static_assert((std::is_same_v<T, std::remove_cv_t<std::remove_reference_t<Args>>> && ...), "All output parameters must be of type T");

    constexpr usize ARGS_COUNT = sizeof...(Args);

    std::array<T, ARGS_COUNT> temp{};

    for (usize i = 0; i < ARGS_COUNT; i++) {
        if (!tryAs<T>(table[i + 1], temp.at(i))) {
            return false;
        }
    }

    usize i = 0;
    auto assign = [&](auto& ref) { ref = std::move(temp.at(i++)); };
    (assign(out), ...);

    return true;
}

/**
 * @brief Extract sequential values from a Lua table(list) by index.
 *
 * This function asserts on debug.
 * It is unsafe in release mode.
 *
 * @tparam T Expected value type.
 * @param table Lua table object.
 * @return A vector containing the extracted values.
 */
template <typename T>
inline T getKey(const sol::table& table, const std::string& key) {
    return as<T>(table[key]);
}

/**
 * @brief Try to extract a value from a Lua table by key.
 *
 * If the key does not exist or the value is not of the expected type, nothing is written to `out`.
 *
 * @tparam T Expected value type.
 * @param table Lua table object.
 * @param key Key to look for in the table.
 * @param out Output reference to store the extracted value.
 * @return True if the value was successfully extracted, false otherwise.
 */
template <typename T>
inline bool tryGetKey(const sol::table& table, const std::string& key, T& out) {
    return tryAs(table[key], out);
}

/**
 * @brief Extract a value from a Lua table by key, or return a default value if the key does not exist.
 *
 * @tparam T Expected value type.
 * @param table Lua table object.
 * @param key Key to look for in the table.
 * @param default_value Default value to return if the key does not exist.
 * @return The value at the specified key, converted to type T, or the default value if the key does not exist.
 */
template <typename T>
inline T getKeyOr(const sol::table& table, const std::string& key, T default_value) {
    if (table[key].valid()) {
        return as<T>(table[key]);
    }
    return default_value;
}

/**
 * @brief Extract a value from a Lua table by index, or return a default value if the index does not exist.
 *
 * @tparam T Expected value type.
 * @param table Lua table object.
 * @param idx Index to look for in the table.
 * @param default_value Default value to return if the index does not exist.
 * @return The value at the specified index, converted to type T, or the default value if the index does not exist.
 */
template <typename T>
inline T getIdxOr(const sol::table& table, int idx, T default_value) {
    if (table[idx].valid()) {
        return as<T>(table[idx]);
    }
    return default_value;
}

/**
 * @brief Try to extract a value from a Lua table by key.
 *
 * @tparam T Expected value type.
 * @param table Lua table object.
 * @param key Key to look for in the table.
 * @return An optional containing the extracted value if it exists and is of type T, or an empty optional otherwise.
 */
template <typename T>
inline std::optional<T> tryGetKey(const sol::table& table, const std::string& key) {
    T ret;
    if (tryGetKey(table, key, ret)) {
        return ret;
    }
    return {};
}

/**
 * @brief Extract a value from a Lua table by index.
 *
 * @tparam T Expected value type.
 * @param table Lua table object.
 * @param idx Index to look for in the table.
 * @return The value at the specified index, converted to type T.
 */
template <typename T>
inline T getIdx(const sol::table& table, int idx) {
    return as<T>(table[idx]);
}

/**
 * @brief Try to extract a value from a Lua table by index.
 *
 * @tparam T Expected value type.
 * @param table Lua table object.
 * @param idx Index to look for in the table.
 * @param out Output reference to store the extracted value.
 * @return True if the value was successfully extracted, false otherwise.
 */
template <typename T>
inline bool tryGetIdx(const sol::table& table, int idx, T& out) {
    return tryAs(table[idx], out);
}

/**
 * @brief Try to get a value from a Lua table by key or index.
 *
 * @tparam T Expected value type.
 * @param table Lua table object.
 * @param key Key to look for in the table.
 * @param idx Index to look for in the table.
 * @param ret Output reference to store the extracted value.
 * @return True if the value was successfully extracted from either key or index, false otherwise.
 */
template <typename T>
inline bool tryGetKeyOrIdx(const sol::table& table, const std::string& key, int idx, T& ret) {
    if (tryGetKey(table, key, ret)) {
        return true;
    }
    if (tryGetIdx(table, idx, ret)) {
        return true;
    }
    return false;
}

/**
 * @brief Try to get a value from a Lua table by key or index.
 *
 * @parem table Lua table object.
 * @param key Key to look for in the table.
 * @param idx Index to look for in the table.
 * @return An optional containing the extracted value if it exists, or an empty optional otherwise.
 */
std::optional<sol::object> getKeyOrIdx(const sol::table& table, const std::string& key, int idx);

/**
 * @brief As overload for `vec2f`. Accepts o as num, vec2f, or a table(list) with 2 numeric elements.
 *
 * This is the unsafe version that asserts the type in debug mode.
 * @see tryAsVec2f for a safer version that checks the type before conversion.
 *
 * @param o Lua object to convert. Can be: num, vec2f, or a table(list) with 2 numeric elements.
 * @return Converted vec2f.
 */
template <>
inline vec2f as(const sol::object& o) {
    MLE_ASSERT(o.valid());

    if (o.is<vec2f>()) {
        return o.as<vec2f>();
    }
    if (o.is<f32>()) {
        return vec2f{o.as<f32>()};
    }
    if (o.is<sol::table>()) {
        vec2f out;
        getList<f32>(o, out[0], out[1]);
        return out;
    }

    MLE_UNREACHABLE_LOG("Cannot convert to vec2f from {}", o.get_type());
}

/**
 * @brief TryAs overload for `vec2f`. Accepts o as num, vec2f, or a table(list) with 2 numeric elements.
 *
 * This is the safe version that checks the type before conversion.
 * @see asVec2f for the unsafe version that asserts the type in debug mode.
 *
 * @param o Lua object to convert. Can be: num, vec2f, or a table(list) with 2 numeric elements.
 * @param out Output reference to store the converted value.
 * @return True if the conversion was successful, false otherwise.
 */
template <>
inline bool tryAs(const sol::object& o, vec2f& out) {
    if (!o.valid()) {
        return false;
    }

    if (o.is<vec2f>()) {
        out = o.as<vec2f>();
        return true;
    }
    if (o.is<f32>()) {
        out = vec2f{o.as<f32>()};
        return true;
    }
    if (o.is<sol::table>()) {
        if (tryGetList<f32>(o, out[0], out[1])) {
            return true;
        }
    }

    return false;
}

/**
 * @brief As overload for `vec2i`. Accepts o as num, vec2i, or a table(list) with 2 integer elements.
 *
 * This is the unsafe version that asserts the type in debug mode.
 * @see tryAs for a safer version that checks the type before conversion.
 *
 * @param o Lua object to convert. Can be: num, vec2i, or a table(list) with 2 integer elements.
 * @return Converted vec2i.
 */
template <>
inline vec2i as(const sol::object& o) {
    MLE_ASSERT(o.valid());

    if (o.is<vec2i>()) {
        return o.as<vec2i>();
    }
    if (o.is<i32>()) {
        return vec2i{o.as<i32>()};
    }
    if (o.is<sol::table>()) {
        vec2i out;
        getList<i32>(o, out[0], out[1]);
        return out;
    }

    MLE_UNREACHABLE_LOG("Cannot convert to vec2i from {}", o.get_type());
}

/**
 * @brief TryAs overload for `vec2i`. Accepts o as num, vec2i, or a table(list) with 2 integer elements.
 *
 * This is the safe version that checks the type before conversion.
 * @see as for the unsafe version that asserts the type in debug mode.
 *
 * @param o Lua object to convert. Can be: num, vec2i, or a table(list) with 2 integer elements.
 * @param out Output reference to store the converted value.
 * @return True if the conversion was successful, false otherwise.
 */
template <>
inline bool tryAs(const sol::object& o, vec2i& out) {
    if (!o.valid()) {
        return false;
    }

    if (o.is<vec2i>()) {
        out = o.as<vec2i>();
        return true;
    }
    if (o.is<i32>()) {
        out = vec2i{o.as<i32>()};
        return true;
    }
    if (o.is<sol::table>()) {
        return tryGetList<i32>(o, out[0], out[1]);
    }

    return false;
}

template <>
inline Recti as(const sol::object& o) {
    MLE_ASSERT(o.valid());

    if (o.is<Recti>()) {
        return o.as<Recti>();
    }
    if (o.is<sol::table>()) {
        Recti ret;
        getList<i32>(o, ret.pos.x, ret.pos.y, ret.size.x, ret.size.y);
        return ret;
    }

    MLE_UNREACHABLE_LOG("Cannot convert to Recti from {}", o.get_type());
}

template <>
inline Rectf as(const sol::object& o) {
    MLE_ASSERT(o.valid());

    if (o.is<Recti>()) {
        return o.as<Rectf>();
    }
    if (o.is<sol::table>()) {
        Rectf ret;
        getList<f32>(o, ret.pos.x, ret.pos.y, ret.size.x, ret.size.y);
        return ret;
    }

    MLE_UNREACHABLE_LOG("Cannot convert to Recti from {}", o.get_type());
}

/**
 * @brief As overload for `vec3f`. Accepts o as num, vec3f, or a table(list) with 3 numeric elements.
 *
 * This is the unsafe version that asserts the type in debug mode.
 * @see tryAs for a safer version that checks the type before conversion.
 *
 * @param o Lua object to convert. Can be: num, vec3f, or a table(list) with 3 numeric elements.
 * @return Converted vec3f.
 */
template <>
inline vec3f as(const sol::object& o) {
    MLE_ASSERT(o.valid());

    if (o.is<vec3f>()) {
        return o.as<vec3f>();
    }
    if (o.is<f32>()) {
        return vec3f{o.as<f32>()};
    }
    if (o.is<sol::table>()) {
        vec3f out;
        getList<f32>(o, out[0], out[1], out[2]);
        return out;
    }

    MLE_UNREACHABLE_LOG("Cannot convert to vec3f from {}", o.get_type());
}

/**
 * @brief TryAs overload for `vec3f`. Accepts o as num, vec3f, or a table(list) with 3 numeric elements.
 *
 * This is the safe version that checks the type before conversion.
 * @see as for the unsafe version that asserts the type in debug mode.
 *
 * @param o Lua object to convert. Can be: num, vec3f, or a table(list) with 3 numeric elements.
 * @param out Output reference to store the converted value.
 * @return True if the conversion was successful, false otherwise.
 */
template <>
inline bool tryAs(const sol::object& o, vec3f& out) {
    if (!o.valid()) {
        return false;
    }

    if (o.is<vec3f>()) {
        out = o.as<vec3f>();
        return true;
    }
    if (o.is<f32>()) {
        out = vec3f{o.as<f32>()};
        return true;
    }
    if (o.is<sol::table>()) {
        return tryGetList<f32>(o, out[0], out[1], out[2]);
    }

    return false;
}

/**
 * @brief As overload for `vec4f`. Accepts o as num, vec4f, or a table(list) with 4 numeric elements.
 *
 * This is the unsafe version that asserts the type in debug mode.
 * @see tryAs for a safer version that checks the type before conversion.
 *
 * @param o Lua object to convert. Can be: num, vec4f, or a table(list) with 4 numeric elements.
 * @return Converted vec4f.
 */
template <>
inline vec4f as(const sol::object& o) {
    MLE_ASSERT(o.valid());

    if (o.is<vec4f>()) {
        return o.as<vec4f>();
    }
    if (o.is<f32>()) {
        return vec4f{o.as<f32>()};
    }
    if (o.is<sol::table>()) {
        vec4f out;
        getList<f32>(o, out[0], out[1], out[2], out[3]);
        return out;
    }

    MLE_UNREACHABLE_LOG("Cannot convert to vec4f from {}", o.get_type());
}

/**
 * @brief TryAs overload for `vec4f`. Accepts o as num, vec4f, or a table(list) with 4 numeric elements.
 *
 * This is the safe version that checks the type before conversion.
 * @see as for the unsafe version that asserts the type in debug mode.
 *
 * @param o Lua object to convert. Can be: num, vec4f, or a table(list) with 4 numeric elements.
 * @param out Output reference to store the converted value.
 * @return True if the conversion was successful, false otherwise.
 */
template <>
inline bool tryAs(const sol::object& o, vec4f& out) {
    if (!o.valid()) {
        return false;
    }

    if (o.is<vec4f>()) {
        out = o.as<vec4f>();
        return true;
    }
    if (o.is<f32>()) {
        out = vec4f{o.as<f32>()};
        return true;
    }
    if (o.is<sol::table>()) {
        return tryGetList<f32>(o, out[0], out[1], out[2], out[3]);
    }

    return false;
}

/**
 * @brief As overload for `mat2f`. Accepts o as num, mat2f, or a table(list) with 4 numeric elements.
 *
 * This is the unsafe version that asserts the type in debug mode.
 * @see tryAs for a safer version that checks the type before conversion.
 *
 * @param o Lua object to convert. Can be: num, mat2f, or a table(list) with 4 numeric elements.
 * @return Converted mat2f.
 */
template <>
inline mat2f as(const sol::object& o) {
    MLE_ASSERT(o.valid());

    if (o.is<mat2f>()) {
        return o.as<mat2f>();
    }
    if (o.is<f32>()) {
        return mat2f{o.as<f32>()};
    }
    if (o.is<sol::table>()) {
        mat2f out;
        getList<f32>(o, out[0][0], out[0][1], out[1][0], out[1][1]);
        return out;
    }

    MLE_UNREACHABLE_LOG("Cannot convert to mat2f from {}", o.get_type());
}

/**
 * @brief TryAs overload for `mat2f`. Accepts o as num, mat2f, or a table(list) with 4 numeric elements.
 *
 * This is the safe version that checks the type before conversion.
 * @see as for the unsafe version that asserts the type in debug mode.
 *
 * @param o Lua object to convert. Can be: num, mat2f, or a table(list) with 4 numeric elements.
 * @param out Output reference to store the converted value.
 * @return True if the conversion was successful, false otherwise.
 */
template <>
inline bool tryAs(const sol::object& o, mat2f& out) {
    if (!o.valid()) {
        return false;
    }

    if (o.is<mat2f>()) {
        out = o.as<mat2f>();
        return true;
    }
    if (o.is<f32>()) {
        out = mat2f{o.as<f32>()};
        return true;
    }
    if (o.is<sol::table>()) {
        return tryGetList<f32>(o, out[0][0], out[0][1], out[1][0], out[1][1]);
    }

    return false;
}

/**
 * @brief As overload for `mat4f`. Accepts o as num, mat4f, or a table(list) with 16 numeric elements.
 *
 * This is the unsafe version that asserts the type in debug mode.
 * @see tryAs for a safer version that checks the type before conversion.
 *
 * @param o Lua object to convert. Can be: num, mat4f, or a table(list) with 16 numeric elements.
 * @return Converted mat4f.
 */
template <>
inline mat4f as(const sol::object& o) {
    MLE_ASSERT(o.valid());

    if (o.is<mat4f>()) {
        return o.as<mat4f>();
    }
    if (o.is<f32>()) {
        return mat4f{o.as<f32>()};
    }
    if (o.is<sol::table>()) {
        mat4f out;
        getList<f32>(o, out[0][0], out[0][1], out[0][2], out[0][3], out[1][0], out[1][1], out[1][2], out[1][3], out[2][0], out[2][1], out[2][2], out[2][3],
                     out[3][0], out[3][1], out[3][2], out[3][3]);
        return out;
    }

    MLE_UNREACHABLE_LOG("Cannot convert to mat4f from {}", o.get_type());
}

/**
 * @brief TryAs overload for `mat4f`. Accepts o as num, mat4f, or a table(list) with 16 numeric elements.
 *
 * This is the safe version that checks the type before conversion.
 * @see as for the unsafe version that asserts the type in debug mode.
 *
 * @param o Lua object to convert. Can be: num, mat4f, or a table(list) with 16 numeric elements.
 * @param out Output reference to store the converted value.
 * @return True if the conversion was successful, false otherwise.
 */
template <>
inline bool tryAs(const sol::object& o, mat4f& out) {
    if (!o.valid()) {
        return false;
    }

    if (o.is<mat4f>()) {
        out = o.as<mat4f>();
        return true;
    }
    if (o.is<f32>()) {
        out = mat4f{o.as<f32>()};
        return true;
    }
    if (o.is<sol::table>()) {
        return tryGetList<f32>(o, out[0][0], out[0][1], out[0][2], out[0][3], out[1][0], out[1][1], out[1][2], out[1][3], out[2][0], out[2][1], out[2][2],
                               out[2][3], out[3][0], out[3][1], out[3][2], out[3][3]);
    }

    return false;
}

void merge(sol::table& dst, const sol::table& src);
}  // namespace mle::lua
