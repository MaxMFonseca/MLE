/**
 * @file Result.h
 * @brief Result codes and helpers for engine operations.
 *
 * Defines a generic Result enum for status reporting across engine components,
 * including utility functions for error classification and string conversion.
 */

#pragma once

#include <expected>
#include <utility>

#include "math/Types.h"

namespace mle {

/// Represents the outcome of an operation.
enum class [[nodiscard]] Result : u8 {
    OK,       ///< Operation succeeded.
    TIMEOUT,  ///< Operation timed out.

    NOK,               ///< Generic Error. Try to avoid. Bad practice! Adding new fields here is easy!
    INVALID_ARGUMENT,  ///< Invalid argument provided.
    NOT_FOUND,         ///< Requested resource not found.
    FAILED_TO_OPEN,    ///< Failed to open a resource (e.g., file, network).
};

/// Converts a Result value to a string literal.
static constexpr const char* toString(Result result) {
    // clang-format off
#define CASE(x) case Result::x: return #x // NOLINT
    // clang-format on
    switch (result) {
        CASE(OK);
        CASE(TIMEOUT);

        CASE(NOK);
        CASE(INVALID_ARGUMENT);
        CASE(NOT_FOUND);
        CASE(FAILED_TO_OPEN);
    }
    std::unreachable();
#undef CASE
}

/// Returns true if the result indicates an error.
static constexpr bool isError(Result result) {
    return result >= Result::NOK;
}

/// A convenience alias for returning a value or a Result error.
template <typename T>
using Expected = std::expected<T, Result>;
}  // namespace mle

namespace fmt {
template <>
struct formatter<mle::Result> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::Result result, FormatContext& ctx) const {
        return format_to(ctx.out(), "{}", toString(result));
    }
};
}  // namespace fmt
