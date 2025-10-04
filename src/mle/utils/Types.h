#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <vector>

#include "mle/math/Types.h"

namespace mle {
using usize = size_t;     ///< Unsigned size type.
using isize = ptrdiff_t;  ///< Signed size type.
using byte = std::byte;   ///< Byte type for raw data.
using char32 = char32_t;  ///< 32-bit Unicode character type.
using UserPtr = void*;    ///< Generic user pointer.

using Bytes = std::vector<u8>;            ///< Dynamic byte array.
using BytesHnd = std::unique_ptr<Bytes>;  ///< Unique handle to a byte buffer.
using BytesRef = Bytes*;                  ///< Raw pointer to a byte buffer.

using ID = u64;                                            ///< 64-bit engine-wide identifier type.
constexpr ID INVALID_ID = std::numeric_limits<ID>::max();  ///< Sentinel value for an invalid ID.
}  // namespace mle

namespace fmt {
template <>
struct formatter<std::filesystem::path> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const std::filesystem::path& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "{}", v.generic_string());
    }
};
}  // namespace fmt
