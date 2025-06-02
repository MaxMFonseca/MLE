/**
 * @file
 * @brief Main renderer interface
 */

#pragma once

#include "Events.h"
#include "Types.h"
#include "mle/common/Result.h"

namespace mle::renderer {
/// Renderer initialization parameters.
struct CreateInfo {};
using CI = CreateInfo;  ///< Alias for renderer CreateInfo.

/**
 * @brief Initializes the renderer with the given parameters.
 * @param ci Renderer creation info.
 * @return Result indicating success or failure.
 */
Result init(const CI& ci);

/// Shuts down the renderer and releases all resources.
void shutdown();

/**
 * @brief Begins a new rendering frame.
 * @return Result indicating success or failure.
 */
Result beginFrame();

/**
 * @brief Ends the current rendering frame and presents the given image.
 * @param image Image to present at the end of the frame.
 */
void endFrame(ImageRef image);

/**
 * @brief Creates a persistent image resource.
 * @param ci Image creation info.
 * @return Owning handle to the created image.
 */
// ImageHnd createImage(const Image::CI& ci);

/**
 * @brief Creates a persistent buffer resource.
 * @param ci Buffer creation info.
 * @return Owning handle to the created buffer.
 */
// ImageHnd createBuffer(const Buffer::CI& ci);

/**
 * @brief Allocates a transient buffer valid only for the current frame.
 * @param ci Buffer creation info.
 * @return Reference to the allocated frame-local buffer.
 */
// BufferRef createBufferOnFrame(const Buffer::CI& ci);

/**
 * @brief Allocates a transient image valid only for the current frame.
 * @param ci Image creation info.
 * @return Reference to the allocated frame-local image.
 */
// ImageRef createImageOnFrame(const Image::CI& ci);

/**
 * @brief Schedules a callback to be called at the end of the current frame.
 * @param func Function to call after the frame ends.
 */
void addToCallOnFrameEnd(std::function<void(void)>&& func);

/**
 * @brief Schedules destruction of an image at the end of the frame.
 * @param image Owning handle to the image.
 */
void destroyOnFrameEnd(ImageHnd&& image);

/**
 * @brief Schedules destruction of a buffer at the end of the frame.
 * @param buffer Owning handle to the buffer.
 */
void destroyOnFrameEnd(BufferHnd&& buffer);

VmaAllocator getVma();     ///< Returns the Vulkan Memory Allocator instance.
vk::Device getVkDevice();  ///< Returns the Vulkan device handle.
}  // namespace mle::renderer
