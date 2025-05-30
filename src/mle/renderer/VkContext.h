/**
 * @file
 * @brief Defines the VkContext class for managing the Vulkan rendering context.
 *
 * The `VkContext` class encapsulates all Vulkan objects required for rendering, including
 * instance, device, surface, swapchain, and memory allocator.
 */

#pragma once

#include <vk_mem_alloc.h>

#include "Utils.h"

namespace mle::renderer {

/**
 * @brief Manages Vulkan context and resources used by the renderer.
 *
 * This class handles the full Vulkan lifecycle, including the Vulkan instance,
 * device creation, memory allocator setup, surface and swapchain management.
 */
class VkContext final {
  public:
    VkContext(const VkContext&) = delete;
    VkContext(VkContext&&) = delete;
    VkContext& operator=(const VkContext&) = delete;
    VkContext& operator=(VkContext&&) = delete;

    /// Constructs an empty VkContext.
    VkContext() = default;

    /// Destroys the VkContext and releases all Vulkan resources.
    ~VkContext() = default;

    /// Initializes the Vulkan instance, device, surface, allocator, and swapchain.
    void init();

    /// Destroys all Vulkan objects owned by this context.
    void shutdown();

    /// (Re)creates the swapchain and associated images.
    void createSwapchain();

    /// Waits until the device has finished all queued operations.
    void waitIdle();

    /// Returns the Vulkan instance.
    [[nodiscard]] vk::Instance getInstance() const { return instance_; }

    /// Returns the logical Vulkan device.
    [[nodiscard]] vk::Device getDevice() const { return device_; }

    /// Returns the selected depth format.
    [[nodiscard]] vk::Format getDepthFormat() const;

    /// Returns the selected color format.
    [[nodiscard]] vk::Format getColorFormat() const;

    /// Returns the index of the all-in-one (graphics/compute) queue.
    [[nodiscard]] usize getAioQueueIdx() const;

    /// Returns the all-in-one (graphics/compute) queue.
    [[nodiscard]] vk::Queue getAioQueue() const;

    /// Returns the index of the transfer-only queue.
    [[nodiscard]] usize getTransferQueueIdx() const;

    /// Returns the transfer-only queue.
    [[nodiscard]] vk::Queue getTransferQueue() const;

    /// Returns the Vulkan Memory Allocator handle.
    [[nodiscard]] VmaAllocator getVma() const;

    /// Returns the swapchain handle.
    [[nodiscard]] vk::SwapchainKHR getSwapchain() const;

    /// Returns the swapchain image format.
    [[nodiscard]] vk::Format getSwapchainFormat() const;

    /// Returns the swapchain extent (width/height).
    [[nodiscard]] vk::Extent2D getSwapchainExtent() const;

    /// Returns the list of swapchain images.
    [[nodiscard]] const std::vector<vk::Image>& getSwapchainImages() const;

    /// Returns a specific swapchain image by index.
    [[nodiscard]] vk::Image getSwapchainImage(u32 idx) const;

    /// Returns the number of swapchain images.
    [[nodiscard]] usize getSwapchainImageCount() const;

    /// Returns the timestamp period in nanoseconds per tick.
    [[nodiscard]] f32 getTimestampPeriod() const;

    /// Returns the minimum alignment for uniform buffer offsets.
    [[nodiscard]] usize getUniformBufferOffsetAlignment() const;

    /// Returns the maximum supported 2D image dimension.
    [[nodiscard]] usize getMaxImageDimension2D() const;

    /// Returns true if the swapchain extent is non-zero.
    [[nodiscard]] bool isSwapchainVisible() const;

  private:
    /// Creates the Vulkan instance.
    void createInstance();

    /// Sets up the Vulkan debug messenger (if enabled).
    void createDebugMessenger();

    /// Creates the rendering surface.
    void createSurface();

    /// Creates the logical Vulkan device and retrieves queues.
    void createDevice();

    /// Initializes the Vulkan Memory Allocator (VMA).
    void createVma();

    /// Creates a command pool and buffer for one-time submissions.
    void createOneTimeSubmitBuffer();

    /// Picks the physical device and stores its properties and features.
    void pickPhysicalDevice();

    /// Selects a suitable depth format.
    void pickDepthFormat();

    /// Selects a suitable color format.
    void pickColorFormat();

    /// Chooses queue family indices for graphics/compute and transfer.
    void pickQueueIdxs();

    /// Logs selected physical device information.
    void logPhysicalDevice();

  private:
    vk::Instance instance_;                       ///< Vulkan instance handle.
    vk::DebugUtilsMessengerEXT debug_messenger_;  ///< Debug messenger (if enabled).
    vk::SurfaceKHR surface_;                      ///< Rendering surface.
    vk::Device device_;                           ///< Logical Vulkan device.

    vk::Queue aio_queue_;       ///< Graphics/compute queue.
    vk::Queue transfer_queue_;  ///< Transfer-only queue.

    VmaAllocator vma_ = nullptr;  ///< Vulkan Memory Allocator handle.

    vk::SwapchainKHR swapchain_;                            ///< Swapchain handle.
    vk::Format swapchain_format_ = vk::Format::eUndefined;  ///< Swapchain image format.
    vk::Extent2D swapchain_extent_;                         ///< Swapchain extent (width and height).
    std::vector<vk::Image> swapchain_images_;               ///< List of swapchain images.

    vk::PhysicalDevice physical_device_;                                              ///< Selected physical device.
    vk::PhysicalDeviceProperties physical_device_properties_;                         ///< Properties of the physical device.
    vk::PhysicalDeviceFeatures physical_device_features_;                             ///< Enabled features of the physical device.
    vk::PhysicalDeviceMemoryProperties physical_device_memory_properties_;            ///< Memory properties of the physical device.
    std::vector<vk::QueueFamilyProperties> physical_device_queue_family_properties_;  ///< Queue family properties.

    vk::Format depth_format_ = {};  ///< Depth image format.
    vk::Format color_format_ = {};  ///< Color image format.

    usize aio_queue_idx_ = max<usize>();       ///< Index of graphics/compute queue family.
    usize transfer_queue_idx_ = max<usize>();  ///< Index of transfer queue family.
};

}  // namespace mle::renderer
