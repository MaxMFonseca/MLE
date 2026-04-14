#pragma once

#include <string>

#include "mle/core/Result.h"
#include "mle/utils/Types.h"
#include "utf8/unchecked.h"

namespace mle {
template <typename T>
Expected<T> strTo(std::string_view s) {
    T value{};

    if (s.empty()) {
        return std::unexpected(Result::INVALID_ARGUMENT);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    const auto* endptr = s.data() + s.size();
    auto [ptr, ec] = std::from_chars(s.data(), endptr, value);

    if (ec == std::errc()) {
        if (ptr == endptr) {
            return value;
        }
        return std::unexpected(Result::INVALID_ARGUMENT);
    }

    if (ec == std::errc::invalid_argument) {
        return std::unexpected(Result::INVALID_ARGUMENT);
    }
    if (ec == std::errc::result_out_of_range) {
        return std::unexpected(Result::OUT_OR_RANGE);
    }

    return std::unexpected(Result::INVALID_ARGUMENT);
}

[[nodiscard]] std::vector<std::string_view> split(std::string_view s, char delim, bool keep_empty = true);

std::string toLower(std::string_view s);
std::string toUpper(std::string_view s);

std::string toUtf8(std::u32string_view s);
std::u32string toUtf32(std::string_view s);

bool isSpace(char c);

std::string_view trim(std::string_view s);

std::pair<f32, std::string_view> splitNumSuffix(std::string_view s);

std::string bitsToString(u64 b);

template <typename... CString>
constexpr bool matchAny(std::string_view str, CString... cs) {
    static_assert((std::is_convertible_v<CString, const char*> && ...), "All arguments must be const char* or convertible.");
    return ((str == cs) || ...);
}
}  // namespace mle

namespace fmt {
template <>
struct formatter<std::u32string> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const std::u32string& s, FormatContext& ctx) const {
        return format_to(ctx.out(), "{}", mle::toUtf8(s));
    }
};
}  // namespace fmt
