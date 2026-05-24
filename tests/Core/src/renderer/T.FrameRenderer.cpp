#include <gtest/gtest.h>

#include <array>
#include <span>

#include "mle/renderer/Types.h"
#include "vulkan/vulkan.hpp"

namespace mle::detail {
vk::SurfaceFormatKHR chooseSwapchainSurfaceFormat(std::span<const vk::SurfaceFormatKHR> available_formats);
}

TEST(FrameRenderer, PrefersSrgbSwapchainFormatForPresentPass) {
    constexpr std::array formats{
        vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear},
        vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
    };

    const auto selected = mle::detail::chooseSwapchainSurfaceFormat(formats);

    EXPECT_EQ(selected.format, vk::Format::eB8G8R8A8Srgb);
    EXPECT_EQ(selected.colorSpace, vk::ColorSpaceKHR::eSrgbNonlinear);
}
