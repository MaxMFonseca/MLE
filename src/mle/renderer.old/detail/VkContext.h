#pragma once

#include "../Types.h"
#include "mle/common/Result.h"
#include "mle/common/Utils.h"

namespace mle::renderer::detail {
class VkContext {
  public:
    struct PhysicalDeviceInfo {
        vk::PhysicalDevice device;
        vk::PhysicalDeviceProperties properties;
        vk::PhysicalDeviceFeatures features;
        vk::PhysicalDeviceMemoryProperties memory_properties;
        std::vector<vk::QueueFamilyProperties> queue_family_properties;
    };

  public:
    VkContext(const VkContext&) = delete;
    VkContext& operator=(const VkContext&) = delete;
    VkContext(VkContext&&) = delete;
    VkContext& operator=(VkContext&&) = delete;

    VkContext() = default;
    ~VkContext() = default;

    void init();

    void waitIdle();

    [[nodiscard]] auto getInstance() { return vk_instance_; }                  ///< Returns the Vulkan instance handle.
    [[nodiscard]] auto getDevice() { return device_; }                         ///< Returns the Vulkan device handle.
    [[nodiscard]] auto getVma() { return vma_; }                               ///< Returns the Vulkan Memory Allocator instance.
    [[nodiscard]] auto getSurface() { return surface_; }                       ///< Returns the Vulkan surface handle.
    [[nodiscard]] auto getPhysicalDevice() const { return p_device_.device; }  ///< Returns the physical device handle.
    [[nodiscard]] auto getColorFormat() const { return color_format_; }        ///< Returns the color format used by the renderer.
    [[nodiscard]] auto getDepthFormat() const { return depth_format_; }        ///< Returns the depth format used by the renderer.

    /// Returns the minimum align for buffers
    [[nodiscard]] u64 getAlignForBufferUsage(vk::BufferUsageFlags flags) const;

    /// Returns the timestamp period of the physical device.
    [[nodiscard]] auto getTimestampPeriod() const { return p_device_.properties.limits.timestampPeriod; }

    /// Returns the dedicated queue index for the specified command type.
    auto getQueueIndex(CmdType type) { return queue_indices_.at(as<usize>(type)); }

    /// Returns all dedicated queues.
    std::array<vk::Queue, 3> getDedicatedQueues();

  private:
    inline void initInstance();
    inline void initDebugMessenger();
    inline void initSurface();
    inline void initDevice();
    inline void pickPhysicalDevice();
    inline void pickQueueIndices();
    inline void initVMA();
    [[nodiscard]] inline Expected<vk::Format> pickFormat(const std::vector<vk::Format>& candidates, vk::FormatFeatureFlags flags) const;

    void logDevice();

  private:
    vk::Instance vk_instance_;
    vk::DebugUtilsMessengerEXT debug_messenger_;
    PhysicalDeviceInfo p_device_;
    vk::Device device_;
    VmaAllocator vma_ = nullptr;
    vk::Format depth_format_ = vk::Format::eUndefined;
    vk::Format color_format_ = vk::Format::eUndefined;

    std::array<usize, 3> queue_indices_ = {max<usize>(), max<usize>(), max<usize>()};
};

}  // namespace mle::renderer::detail
