#pragma once
#include "Types.h"

namespace mle::renderer {
constexpr auto FRAMES_IN_FLIGHT = 2;
constexpr u64 DEFAULT_TIMEOUT_NS = 1'000'000'000;         ///< Default timeout for Vulkan operations in nanoseconds.
constexpr int NO_FRAME = -1;                              ///< Constant representing no current frame being rendered.
constexpr u64 MIN_FRAME_HOST_VISIBLE_SIZE = 16 * 1024UL;  ///< Minimum size for host-visible buffers in frames.
}  // namespace mle::renderer
