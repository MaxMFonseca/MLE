/**
 * @file
 * @brief Main renderer interface
 */

#pragma once

#include "Events.h"
#include "Types.h"
#include "mle/common/Result.h"
#include "mle/renderer/detail/TextureCache.h"

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

void deleteAfterFrame(ImageHnd&& image);  ///< Schedules an image for destruction at the end of the frame.

void deleteAfterFrame(BufferHnd&& buffer);  ///< Schedules a buffer for destruction at the end of the frame.

vk::Format getDefaultColorFormat();  ///< Returns the default image format used by the renderer.

vk::Format getDepthFormat();  ///< Returns the default depth format used by the renderer.

ShaderRef getShader(const std::string& name);  ///< Returns a shader reference by name, loading it if necessary.

Texture addTexture(const std::string& name, ImageHnd&& img);  ///< Gives an image to the texture cache, allowing it to be used by name.

Texture getTexture(const std::string& name);  ///< Returns a texture reference by name, loading it if necessary.

Expected<Model> getModel(const std::string& name);  ///< Returns a model reference by name, loading it if necessary.

void loadModel(const std::string& name, std::function<void(Model)>&& callback = {});  ///< Requests loading a model by name.

void enqueueTextureUpdateJob(TextureUpdateJobData&& data);

vk::DescriptorSetLayout getTexturesDescriptorSetLayout();  ///< Returns the descriptor set layout used for textures.

void bindTexturesDSet(RenderingThread& thread);

vk::CommandBuffer getOTSCmd(CmdType cmd_type);
void submitOTSWait(CmdType cmd_type, vk::CommandBuffer cmd);
void submitOTSAsync(CmdType cmd_type, vk::CommandBuffer cmd, std::function<void(void)>&& callback = {});
void submitOTSAsync(CmdType cmd_type, vk::SubmitInfo2 submit_info, std::function<void(void)>&& callback = {});

vk::CommandBuffer getFrameSecondaryCmd();
void submitJobOnFrame(vk::CommandBuffer cmd);

namespace detail {
ED& getED();                                  ///< Returns the event dispatcher instance for the renderer.
VkContext& getVk();                           ///< Returns the Vulkan context instance.
vk::Device getDevice();                       ///< Returns the Vulkan device handle.
VmaAllocator getVma();                        ///< Returns the Vulkan Memory Allocator instance.
FencePool& getFencePool();                    ///< Returns the fence pool for managing Vulkan fences.
CommandPoolManager& getCommandPoolManager();  ///< Returns the command pool manager.
FrameRenderer& getFrameRenderer();            ///< Returns the frame renderer instance.
void waitIdle();                              ///< Waits for the Vulkan device to become idle, ensuring all operations are complete.
vk::CommandBuffer getFramePrimaryCmd();
}  // namespace detail

template <typename T>
void destroy(T t) {
    detail::getDevice().destroy(t);
}
}  // namespace mle::renderer
