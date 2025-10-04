/**
 * @file Types.h
 * @brief Core type definitions and utilities used across the engine.
 */

#pragma once

#include "mle/math/Types.h"

using namespace std::chrono_literals;  // NOLINT(google-global-names-in-headers) chrono_literals is fine

namespace mle {
/// Severity levels for engine logging.
enum class LogLevel : i8 {
    TRACE,     ///< Verbose debugging information. Example: Logging function calls and variable values.
    DEBUG,     ///< General debugging information. Default level in debug builds.
    INFO,      ///< Informational events. Example: system startup, selected GPU. Default in release builds.
    WARN,      ///< Potential issues that may require attention but are not errors.
    ERROR,     ///< Recoverable errors or major degradations. Example: asset fallback used.
    CRITICAL,  ///< Unrecoverable errors or fatal faults. Example: out of memory, system init failed.
    OFF        ///< Logging is disabled.
};
}  // namespace mle

namespace fmt {
template <>
struct formatter<mle::LogLevel> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::LogLevel v, FormatContext& ctx) const {
        switch (v) {
            case mle::LogLevel::TRACE:
                return format_to(ctx.out(), "TRACE");
            case mle::LogLevel::DEBUG:
                return format_to(ctx.out(), "DEBUG");
            case mle::LogLevel::INFO:
                return format_to(ctx.out(), "INFO");
            case mle::LogLevel::WARN:
                return format_to(ctx.out(), "WARN");
            case mle::LogLevel::ERROR:
                return format_to(ctx.out(), "ERROR");
            case mle::LogLevel::CRITICAL:
                return format_to(ctx.out(), "CRITICAL");
            case mle::LogLevel::OFF:
                return format_to(ctx.out(), "OFF");
            default:
                return format_to(ctx.out(), "UNKNOWN");
        }
    }
};
}  // namespace fmt
