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
 */
void init(const CI& ci);

/// Shuts down the renderer and releases all resources.
void shutdown();

/// Updates the renderer state, processing events and performing necessary updates.
void update();

/**
 * @brief Adds a function to be called on renderer shutdown.
 *
 * @param func Function to call on shutdown.
 */
void addOnShutdown(std::function<void(void)>&& func);

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
 * @brief Adds a function to be called at the end of the current frame.
 * @param func Function to call at the end of the frame.
 */
void addToCallOnFrameEnd(std::function<void()>&& fn);

vk::Format getDefaultColorFormat();  ///< Returns the default image format used by the renderer.

ShaderRef getShader(const std::string& name, bool engine = false);  /// Returns a shader reference by name, loading it if necessary.

Texture getTexture(const std::string& name, bool engine = false);  ///< Returns a texture reference by name, loading it if necessary.

u32 useTexture(RenderingThread& thread, u32 idx);  ///< Write texture and returns the index of the texture in the current frame.

vk::DescriptorSetLayout getTexturesDescriptorSetLayout();  ///< Returns the descriptor set layout used for textures.

void bindTexturesDSet(RenderingThread& thread);

vk::CommandBuffer getOTSCmd(CmdType cmd_type);
void submitOTSWait(CmdType cmd_type, vk::CommandBuffer cmd);
void submitOTSAsync(CmdType cmd_type, vk::CommandBuffer cmd, std::function<void(void)>&& callback = {});
void submitOTSAsync(CmdType cmd_type, vk::SubmitInfo2 submit_info, std::function<void(void)>&& callback = {});

namespace detail {
ED& getED();                                  ///< Returns the event dispatcher instance for the renderer.
VkContext& getVk();                           ///< Returns the Vulkan context instance.
vk::Device getDevice();                       ///< Returns the Vulkan device handle.
VmaAllocator getVma();                        ///< Returns the Vulkan Memory Allocator instance.
FencePool& getFencePool();                    ///< Returns the fence pool for managing Vulkan fences.
CommandPoolManager& getCommandPoolManager();  ///< Returns the command pool manager.
FrameRenderer& getFrameRenderer();            ///< Returns the frame renderer instance.
void waitIdle();                              ///< Waits for the Vulkan device to become idle, ensuring all operations are complete.
}  // namespace detail

// /**
//  * @brief Creates a persistent image resource.
//  * @param ci Image creation info.
//  * @return Owning handle to the created image.
//  */
// // ImageHnd createImage(const Image::CI& ci);
//
// /**
//  * @brief Creates a persistent buffer resource.
//  * @param ci Buffer creation info.
//  * @return Owning handle to the created buffer.
//  */
// // ImageHnd createBuffer(const Buffer::CI& ci);
//
// /**
//  * @brief Allocates a transient buffer valid only for the current frame.
//  * @param ci Buffer creation info.
//  * @return Reference to the allocated frame-local buffer.
//  */
// // BufferRef createBufferOnFrame(const Buffer::CI& ci);
//
// /**
//  * @brief Allocates a transient image valid only for the current frame.
//  * @param ci Image creation info.
//  * @return Reference to the allocated frame-local image.
//  */
// // ImageRef createImageOnFrame(const Image::CI& ci);
//
// /**
//  * @brief Schedules a callback to be called at the end of the current frame.
//  * @param func Function to call after the frame ends.
//  */
// void addToCallOnFrameEnd(std::function<void(void)>&& func);
//
// /**
//  * @brief Schedules destruction of an image at the end of the frame.
//  * @param image Owning handle to the image.
//  */
// void destroyOnFrameEnd(ImageHnd&& image);
//
// /**
//  * @brief Schedules destruction of a buffer at the end of the frame.
//  * @param buffer Owning handle to the buffer.
//  */
// void destroyOnFrameEnd(BufferHnd&& buffer);
//
// VmaAllocator getVma();   ///< Returns the Vulkan Memory Allocator instance.
// vk::Device getDevice();  ///< Returns the Vulkan device handle.
//
// /**
//  * @brief Acquires a command buffer of the specified type.
//  *
//  * @param type The type of command buffer to acquire.
//  * @return A command buffer handle.
//  */
// vk::CommandBuffer acquireCmdBufferRaw(CmdType type);
//
// /**
//  * @brief Submits a command buffer and waits for it to finish.
//  *
//  * @param cmd The command buffer to submit.
//  * @param type The type of command buffer.
//  */
// void submitWait(vk::CommandBuffer cmd, CmdType type);
//
// /**
//  * @brief Submits a command buffer asynchronously.
//  *
//  * @param cmd The command buffer to submit.
//  * @param type The type of command buffer.
//  * @param callback A callback to be called after the command buffer is processed.
//  */
// void submitAsync(vk::CommandBuffer cmd, CmdType type, std::function<void(void)>&& callback);
//
// /**
//  * @brief Releases a command buffer back to the pool.
//  *
//  * @param cmd The command buffer to release.
//  * @param type The type of command buffer.
//  */
// void release(vk::CommandBuffer cmd, CmdType type);
}  // namespace mle::renderer
