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
    OK,                   ///< Operation succeeded.
    FRAME_SKIP,           ///< Frame was intentionally skipped.
    IMAGE_COUNT_CHANGED,  ///< Swapchain image count changed.
    TEXTURE_PENDING,      ///< Texture upload is pending.

    UNKNOWN_ERROR,        ///< An unknown error occurred.
    NOK,                  ///< A generic error occurred. Bad practice to use this. To be removed.
    PIPELINE_NOT_BUILT,   ///< A pipeline was not built before use.
    INIT_FAILED,          ///< Initialization failed.
    RESOURCE_NOT_FOUND,   ///< A required resource was not found.
    ELEMENT_NOT_FOUND,    ///< An expected element was missing.
    FAILED_TO_OPEN_FILE,  ///< Failed to open a file.
    INVALID_ARGUMENT,     ///< One or more arguments were invalid.
    NOT_IMPLEMENTED,      ///< Functionality is not yet implemented.
    NOT_SUPPORTED,        ///< Feature is not supported.
    NOT_READY,            ///< Operation cannot proceed yet.
    NOT_INITIALIZED,      ///< Component has not been initialized.
    NOT_FOUND,            ///< Generic not-found result.
    VK_ERROR,             ///< Vulkan-specific error occurred.
};

/// Converts a Result value to a string literal.
static constexpr const char* toString(Result result) {
    // clang-format off
#define CASE(x) case Result::x: return #x // NOLINT
    // clang-format on
    switch (result) {
        CASE(OK);
        CASE(FRAME_SKIP);
        CASE(IMAGE_COUNT_CHANGED);
        CASE(TEXTURE_PENDING);

        CASE(UNKNOWN_ERROR);
        CASE(NOK);
        CASE(PIPELINE_NOT_BUILT);
        CASE(INIT_FAILED);
        CASE(RESOURCE_NOT_FOUND);
        CASE(ELEMENT_NOT_FOUND);
        CASE(FAILED_TO_OPEN_FILE);
        CASE(INVALID_ARGUMENT);
        CASE(NOT_IMPLEMENTED);
        CASE(NOT_SUPPORTED);
        CASE(NOT_READY);
        CASE(NOT_INITIALIZED);
        CASE(NOT_FOUND);
        CASE(VK_ERROR);
    }
    std::unreachable();
#undef CASE
}

/// Returns true if the result indicates an error.
static constexpr bool isError(Result result) {
    return result >= Result::UNKNOWN_ERROR;
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
