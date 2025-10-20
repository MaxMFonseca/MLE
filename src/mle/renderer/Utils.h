#pragma once

#include "Types.h"
#include "mle/core/Core.h"
#include "mle/utils/Color.h"
#include "mle/utils/Utils.h"

namespace mle {
inline bool isVkError(vk::Result result) {
    return as<i64>(result) < 0;
}

inline void check(vk::Result result) {
    Core::check(!isVkError(result), "Vulkan error: {}", result);
}

inline void check(VkResult result) {
    check(vk::Result(result));
}

template <typename T>
[[nodiscard]] T unwrap(vk::ResultValue<T>&& rv) {
    check(vk::Result(rv.result));
    return std::move(rv).value;
}

inline vk::Extent2D toVkExtent2D(const vec2u& extent) {
    return {extent.x, extent.y};
}

inline vk::Extent3D toVkExtent3D(const vec2u& extent, u32 d = 1) {
    return {extent.x, extent.y, d};
}

inline vk::ClearColorValue toVkColor(const Color& color) {
    return vk::ClearColorValue(std::array<f32, 4>{color.r, color.g, color.b, color.a});
}

constexpr u64 alignUp(u64 value, u64 alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

}  // namespace mle
