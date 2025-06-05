#pragma once
#include "Types.h"
namespace mle::renderer {
constexpr auto FRAMES_IN_FLIGHT = 2;
constexpr u64 DEFAULT_TIMEOUT_NS = 1'000'000'000;  ///< Default timeout for Vulkan operations in nanoseconds.
constexpr int NO_FRAME = -1;                       ///< Constant representing no current frame being rendered.
}  // namespace mle::renderer
