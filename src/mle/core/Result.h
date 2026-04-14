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

#include "mle/math/Types.h"

namespace mle {

/// Represents the outcome of an operation.
enum class [[nodiscard]] Result : u8 {
    OK,                        ///< Operation succeeded.
    TIMEOUT,                   ///< Operation timed out.
    SWAPCHAIN_NOT_VISIBLE,     ///< Swapchain is not visible, e.g., window minimized or not focused.
    FRAME_SKIP,                ///< Frame was skipped, e.g., due to swapchain not being visible.
    NOT_READY,                 ///< Resource or operation not ready yet, e.g., waiting for GPU.
    CURSOR_NOT_INSIDE_WINDOW,  ///< Cursor is not inside the window.
    DISABLED,                  ///< Feature is disabled.

    NOK,                ///< Generic Error. Try to avoid. Bad practice! Adding new fields here is easy!
    INVALID_ARGUMENT,   ///< Invalid argument provided.
    OUT_OR_RANGE,       ///< Value out of acceptable range.
    NOT_FOUND,          ///< Requested resource not found.
    FAILED_TO_OPEN,     ///< Failed to open a resource (e.g., file, network).
    ALLOCATION_FAILED,  ///< Memory allocation failed.
    FAILED_TO_CREATE,   ///< Failed to create a resource (e.g., texture, buffer).
    INVALID_TYPE,       ///< Invalid type encountered.
    OAL_ERROR,          ///< OpenAL error occurred.
};

/// Converts a Result value to a string literal.
static constexpr const char* toString(Result result) {
    // clang-format off
#define CASE(x) case Result::x: return #x // NOLINT
    // clang-format on
    switch (result) {
        CASE(OK);
        CASE(TIMEOUT);
        CASE(SWAPCHAIN_NOT_VISIBLE);
        CASE(FRAME_SKIP);
        CASE(NOT_READY);
        CASE(CURSOR_NOT_INSIDE_WINDOW);
        CASE(DISABLED);

        CASE(NOK);
        CASE(INVALID_ARGUMENT);
        CASE(OUT_OR_RANGE);
        CASE(NOT_FOUND);
        CASE(FAILED_TO_OPEN);
        CASE(ALLOCATION_FAILED);
        CASE(FAILED_TO_CREATE);
        CASE(INVALID_TYPE);
        CASE(OAL_ERROR);
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
