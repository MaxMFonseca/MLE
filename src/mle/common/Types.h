/**
 * @file Types.h
 * @brief Core type definitions and utilities used across the engine.
 */

#pragma once

#include <filesystem>
#include <utility>
#include <vector>

#include "math/Types.h"

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

using usize = size_t;     ///< Unsigned size type.
using isize = ptrdiff_t;  ///< Signed size type.
using byte = std::byte;   ///< Byte type for raw data.
using UserPtr = void*;    ///< Generic user pointer.
using char32 = char32_t;  ///< 32-bit Unicode character type.

using Bytes = std::vector<byte>;          ///< Dynamic byte array.
using BytesHnd = std::unique_ptr<Bytes>;  ///< Unique handle to a byte buffer.
using BytesRef = Bytes*;                  ///< Raw pointer to a byte buffer.

class UnicodeBuffer;                      ///< Forward declaration for Unicode-capable buffer class.
using UnicodeBufferRef = UnicodeBuffer*;  ///< Raw pointer to a UnicodeBuffer instance.

using ID = u64;                                            ///< 64-bit engine-wide identifier type.
constexpr ID INVALID_ID = std::numeric_limits<ID>::max();  ///< Sentinel value for an invalid ID.

template <typename T>
using IDVec = std::vector<std::pair<ID, T>>;  ///< A vector of (ID, T) pairs used for indexed collections.

namespace fs = std::filesystem;  ///< Alias for the filesystem namespace. Remeber to use nothrow overloads!
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

template <>
struct formatter<mle::fs::path> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::fs::path& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "{}", v.generic_string());
    }
};

}  // namespace fmt
