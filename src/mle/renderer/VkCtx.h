#pragma once

#include <mutex>

#include "Utils.h"
#include "mle/utils/Utils.h"

namespace mle {
class VkCtx {
  public:
    struct PhysicalDeviceInfo {
        vk::PhysicalDevice o;
        vk::PhysicalDeviceProperties properties;
        vk::PhysicalDeviceFeatures features;
        vk::PhysicalDeviceMemoryProperties memory_properties;
        std::vector<vk::QueueFamilyProperties> queue_family_properties;
    };

    struct QueueData {
        static constexpr u32 INVALID_FAMILY = max<u32>();
        u32 g_fam_idx = INVALID_FAMILY;
        u32 c_fam_idx = INVALID_FAMILY;
        u32 t_fam_idx = INVALID_FAMILY;

        vk::Queue g_queue;
        vk::Queue c_queue;
        vk::Queue t_queue;

        bool separate_compute = false;
        bool dedicated_transfer = false;
    };

  public:
    MLE_NO_COPY_MOVE(VkCtx);
    ~VkCtx() = default;

    void init();
    void shutdown();

    [[nodiscard]] bool isSurfaceUNORM() const;

    const auto& getQueueData() { return queue_data_; }

    auto getDevice() { return device_; }
    auto dev() { return device_; }
    auto getVma() { return vma_; }

    template <class T>
    void destroy(T o) {
        device_.destroy(o);
    }

    [[nodiscard]] vk::DeviceSize getAlignmentForBufferUsage(vk::BufferUsageFlags flags) const;
    [[nodiscard]] auto getVkImageFormat(ImageFormat format) const { return image_formats_.at(as<usize>(format)); }
    [[nodiscard]] auto getVkImageUsage(ImageFormat format) const { return image_format_usages_.at(as<usize>(format)); }
    [[nodiscard]] const auto& getPhysicalDevice() const { return p_device_; }
    [[nodiscard]] const auto& getInstance() const { return vk_instance_; }
    [[nodiscard]] const auto& getSurface() const { return surface_; }

  private:
    friend Renderer;
    VkCtx() = default;

  private:
    inline void initInstance();
    inline void initDebugMessenger();
    inline void initSurface();
    inline void pickPhysicalDevice();
    inline void pickQueueIndices();
    inline void pickImageFormats();
    inline void initDevice();
    inline void initVMA();

    void logDevice();

  private:
    vk::Instance vk_instance_;
    vk::DebugUtilsMessengerEXT debug_messenger_;
    vk::SurfaceKHR surface_;
    PhysicalDeviceInfo p_device_;

    vk::Device device_;
    VmaAllocator vma_{};

    std::array<vk::Format, as<usize>(ImageFormat::COUNT)> image_formats_{};
    std::array<vk::ImageUsageFlags, as<usize>(ImageFormat::COUNT)> image_format_usages_{};

    QueueData queue_data_{};
};
}  // namespace mle
