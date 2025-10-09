#pragma once

#define VULKAN_HPP_ASSERT(expr) ((void)0)
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_EXCEPTIONS

#include <vk_mem_alloc.h>

#include <vulkan/vulkan.hpp>

#include "mle/math/Types.h"

namespace mle {
enum class GCmdType : u8 { GRAPHICS = 0, COMPUTE = 1, TRANSFER = 2, G = GRAPHICS, C = COMPUTE, T = TRANSFER };
}  // namespace mle

namespace fmt {
template <>
struct formatter<vk::Result> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(vk::Result result, FormatContext& ctx) const {
        return format_to(ctx.out(), "{}", vk::to_string(result));
    }
};

template <>
struct formatter<vk::Extent2D> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(vk::Extent2D const& extent, FormatContext& ctx) const {
        return format_to(ctx.out(), "[{}, {}]", extent.width, extent.height);
    }
};

template <>
struct formatter<vk::Format> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(vk::Format format, FormatContext& ctx) const {
        return format_to(ctx.out(), "{}", vk::to_string(format));
    }
};

template <>
struct formatter<mle::GCmdType> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::GCmdType type, FormatContext& ctx) const {
        switch (type) {
            case mle::GCmdType::GRAPHICS:
                return format_to(ctx.out(), "GRAPHICS");
            case mle::GCmdType::COMPUTE:
                return format_to(ctx.out(), "COMPUTE");
            case mle::GCmdType::TRANSFER:
                return format_to(ctx.out(), "TRANSFER");
            default:
                return format_to(ctx.out(), "UNKNOWN");
        }
    }
};

}  // namespace fmt
