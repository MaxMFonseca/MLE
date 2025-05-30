/**
 * @file
 * @brief Lua types and their stringification utilities.
 */

#pragma once

#include <spdlog/fmt/fmt.h>

#include <sol/sol.hpp>

namespace mle::lua {
/**
 * @brief Converts a Lua object to a string representation with an optional name.
 *
 * @param obj Lua object to stringify.
 * @return A formatted string representing the Lua object.
 */
std::string toString(const sol::object& obj, const std::string& prefix = "");
}  // namespace mle::lua

namespace fmt {
using namespace mle::lua;  // NOLINT safe enclosed

template <>
struct formatter<sol::type> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(sol::type const type, FormatContext& ctx) const {
        switch (type) {
            case sol::type::none:
                return format_to(ctx.out(), "none");
            case sol::type::nil:
                return format_to(ctx.out(), "nil");
            case sol::type::number:
                return format_to(ctx.out(), "number");
            case sol::type::string:
                return format_to(ctx.out(), "string");
            case sol::type::function:
                return format_to(ctx.out(), "function");
            case sol::type::table:
                return format_to(ctx.out(), "table");
            case sol::type::userdata:
                return format_to(ctx.out(), "userdata");
            case sol::type::thread:
                return format_to(ctx.out(), "thread");
            case sol::type::lightuserdata:
                return format_to(ctx.out(), "lightuserdata");
            case sol::type::boolean:
                return format_to(ctx.out(), "boolean");
            default:
                return format_to(ctx.out(), "<unknown>");
        }
    }
};

template <>
struct formatter<sol::table> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const sol::table& obj, FormatContext& ctx) const {
        return format_to(ctx.out(), "table:\n{}", toString(obj));
    }
};

template <>
struct formatter<sol::object> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const sol::object& obj, FormatContext& ctx) const {
        return format_to(ctx.out(), "{}", toString(obj));
    }
};
}  // namespace fmt
