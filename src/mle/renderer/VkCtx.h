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
        static constexpr usize INVALID_FAMILY = max<usize>();
        usize g_fam_idx = INVALID_FAMILY;
        usize c_fam_idx = INVALID_FAMILY;
        usize t_fam_idx = INVALID_FAMILY;

        vk::Queue g_queue;
        vk::Queue c_queue;
        vk::Queue t_queue;

        bool separate_compute = false;
        bool dedicated_transfer = false;
    };

  public:
    MLE_NO_COPY_MOVE(VkCtx);
    ~VkCtx() { shutdown(); }

    void init();
    void shutdown();

    [[nodiscard]] bool isSurfaceUNORM() const;

    const auto& getQueueData() { return queue_data_; }

    auto getDevice() { return device_; }
    auto dev() { return device_; }

    template <class T>
    void destroy(T o) {
        device_.destroy(o);
    }

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

    struct {
        vk::Format swapchain;
        vk::Format depth;
        vk::Format texture4u, texture4srgb, texture2u, texture1u;
        vk::Format gbuf_params;
        vk::Format normals;
        vk::Format color;
        vk::Format storage;
    } formats_{};

    QueueData queue_data_{};
};
}  // namespace mle
