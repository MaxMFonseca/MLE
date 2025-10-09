#pragma once

#include "Utils.h"
#include "mle/utils/Utils.h"

namespace mle {
class VkCtx {
    MLE_SINGLETON(VkCtx)

  public:
    struct PhysicalDeviceInfo {
        vk::PhysicalDevice o;
        vk::PhysicalDeviceProperties properties;
        vk::PhysicalDeviceFeatures features;
        vk::PhysicalDeviceMemoryProperties memory_properties;
        std::vector<vk::QueueFamilyProperties> queue_family_properties;
    };

  public:
    void init();
    void shutdown();

    [[nodiscard]] bool isSurfaceUNORM() const;

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

    struct RenderFormats {
        vk::Format swapchain;
        vk::Format depth;
        vk::Format texture4u, texture4srgb, texture2u, texture1u;
        vk::Format gbuf_params;
        vk::Format normals;
        vk::Format color;
        vk::Format storage;
    } formats_{};

    int g_queue_family_index_ = -1;
    int c_queue_family_index_ = -1;
    int t_queue_family_index_ = -1;

    vk::Queue g_queue_;
    vk::Queue c_queue_;
    vk::Queue t_queue_;

    bool separate_compute_queue_ = false;
    bool dedicated_transfer_queue_ = false;
};
}  // namespace mle
