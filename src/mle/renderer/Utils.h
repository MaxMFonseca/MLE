#pragma once

#include "Types.h"
#include "mle/core/Core.h"
#include "mle/utils/Utils.h"

namespace mle {
inline bool isVkError(vk::Result result) {
    return as<i64>(result) < 0;
}

inline void check(vk::Result result) {
    if (isVkError(result)) {
        Core::i().unrecoverable("Vulkan error: {}", result);
    }
}

template <typename T>
[[nodiscard]] T unwrap(vk::ResultValue<T>&& rv) {
    check(vk::Result(rv.result));
    return std::move(rv).value;
}

inline vk::Extent2D toVkExtent2D(const vec2i& extent) {
    return {as<u32>(extent.x), as<u32>(extent.y)};
}

}  // namespace mle
