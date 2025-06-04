#include "Renderer.h"

#include <vulkan/vk_enum_string_helper.h>

#include <ranges>

#include "Buffer.h"
#include "detail/FencePool.h"
#include "detail/VkContext.h"
#include "mle/common/Assert.h"
#include "mle/core/Core.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/detail/CommandPoolManager.h"
#include "mle/window/Window.h"

namespace mle::renderer {
namespace {
struct FrameData {
    vk::Semaphore image_available_semaphore;
    vk::Semaphore render_finished_semaphore;
    vk::Fence render_finished_fence;
    vk::CommandPool command_pool;
    vk::CommandBuffer cmd;
    vk::QueryPool query_pool;

    std::vector<std::function<void(void)>> delete_queue;
    std::vector<BufferHnd> buffers;
    std::vector<ImageHnd> images;
};

class Impl {
  public:
    // TODO: State

  public:
    inline void init();
    void shutdown();
    void addOnShutdown(std::function<void(void)>&& func);

    auto& getVk() { return vk_; }                 ///< Returns the Vulkan context instance.
    auto getDevice() { return vk_.getDevice(); }  ///< Returns the Vulkan device handle.
    auto getVma() { return vk_.getVma(); }        ///< Returns the Vulkan Memory Allocator instance.
    auto& getFencePool() { return fence_pool_; }  ///< Returns the fence pool for managing Vulkan fences.

  private:
  private:
    ED ed_;

    detail::VkContext vk_;
    detail::CommandPoolManager command_pool_manager_;
    detail::FencePool fence_pool_;

    std::vector<std::function<void(void)>> shutdown_delete_queue_;

    // std::vector<ImageHnd> swapchain_images_;

    // usize frame_number_ = 0;
    // u8 current_frame_idx_ = 0;
    // u8 current_swapchain_image_idx_ = max<u8>();
    // std::array<FrameData, 2> frames_;
    //
    // bool swapchain_dirty_ = true;
    // bool frame_active_ = false;
    //
    // vk::CommandPool transfer_ots_command_pool_;
    // vk::CommandBuffer transfer_ots_cmd_;
    // vk::Fence transfer_ots_fence_;
    //
    //
    // ImageHnd default_texture_;

    // bool at_render_pass_ = false;
    // Recti current_render_area_{};
    // PipelineRef current_pipeline_{};
};
std::unique_ptr<Impl> i_;  // NOLINT

void Impl::init() {
    MLE_I("Initializing the Renderer");

    vk_.init();

    command_pool_manager_.init();

    MLE_I("Renderer initialized successfully!");
}

void Impl::shutdown() {
    MLE_I("Cleaning up Renderer");

    vk_.waitIdle();

    for (auto& it : std::ranges::reverse_view(shutdown_delete_queue_)) {
        it();
    }

    MLE_D("Live instances after shutdown:");
    LiveCounter<Buffer>::listActiveInstances("Buffer");
    LiveCounter<Image>::listActiveInstances("Image");
}

void Impl::addOnShutdown(std::function<void(void)>&& func) {
    shutdown_delete_queue_.emplace_back(std::move(func));
}

// vk::SurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats, vk::SurfaceFormatKHR target_format) {
//     for (const auto& format : available_formats) {
//         if (format.format == target_format.format && format.colorSpace == target_format.colorSpace) {
//             return format;
//         }
//     }
//
//     return available_formats[0];
// }
//
// Result Impl::createSwapchain() {
//     MLE_D("Creating swapchain");
//
//     auto old_swapchain = swapchain_;
//
//     auto surface_capabilities_r = p_device_.device.getSurfaceCapabilitiesKHR(surface_);
//     if (surface_capabilities_r.result != vk::Result::eSuccess) {
//         core::unrecoverable("Failed to get surface capabilities! Error code: {}", surface_capabilities_r.result);
//     }
//     auto surface_formats_r = p_device_.device.getSurfaceFormatsKHR(surface_);
//     if (surface_formats_r.result != vk::Result::eSuccess) {
//         core::unrecoverable("Failed to get surface formats! Error code: {}", surface_formats_r.result);
//     }
//     auto surface_present_modes_r = p_device_.device.getSurfacePresentModesKHR(surface_);
//     if (surface_present_modes_r.result != vk::Result::eSuccess) {
//         core::unrecoverable("Failed to get surface present modes! Error code: {}", surface_present_modes_r.result);
//     }
//
//     auto& surface_capabilities = surface_capabilities_r.value;
//     auto& surface_formats = surface_formats_r.value;
//     [[maybe_unused]] auto& surface_present_modes = surface_present_modes_r.value;
//
//     auto swapchain_extent = surface_capabilities.currentExtent;
//     auto surface_format = chooseSwapchainSurfaceFormat(surface_formats, vk::SurfaceFormatKHR{color_format_, vk::ColorSpaceKHR::eSrgbNonlinear});
//
// #ifdef MLE_RUNTIME_CONFIG_PRESENT_MODE
//     auto surface_present_mode = runtime_config::getPresentMode();
// #else
//     auto surface_present_mode = vk::PresentModeKHR::eFifo;
// #endif
//
//     usize image_count = surface_capabilities.minImageCount + 1;
//     if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) {
//         image_count = surface_capabilities.maxImageCount;
//     }
//
//     vk::SwapchainCreateInfoKHR swapchain_ci{};
//     swapchain_ci.surface = surface_;
//     swapchain_ci.minImageCount = image_count;
//     swapchain_ci.imageFormat = surface_format.format;
//     swapchain_ci.imageColorSpace = surface_format.colorSpace;
//     swapchain_ci.imageExtent = swapchain_extent;
//     swapchain_ci.imageArrayLayers = 1;
//     swapchain_ci.imageUsage = vk::ImageUsageFlagBits::eTransferDst;
//     swapchain_ci.preTransform = surface_capabilities.currentTransform;
//     swapchain_ci.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
//     swapchain_ci.presentMode = surface_present_mode;
//     swapchain_ci.clipped = vk::True;
//     swapchain_ci.oldSwapchain = old_swapchain;
//     swapchain_ci.imageSharingMode = vk::SharingMode::eExclusive;
//     swapchain_ci.queueFamilyIndexCount = 0;
//     swapchain_ci.pQueueFamilyIndices = nullptr;
//
//     auto swapchain_create_r = device_.createSwapchainKHR(swapchain_ci);
//     if (swapchain_create_r.result != vk::Result::eSuccess) {
//         core::unrecoverable("Failed to create swapchain! Error code: {}", swapchain_create_r.result);
//     }
//     swapchain_ = swapchain_create_r.value;
//
//     if (old_swapchain) {
//         swapchain_images_.clear();
//         device_.destroy(old_swapchain);
//     }
//
//     auto swapchain_images_r = device_.getSwapchainImagesKHR(swapchain_);
//     if (swapchain_images_r.result != vk::Result::eSuccess) {
//         core::unrecoverable("Failed to get swapchain images! Error code: {}", swapchain_images_r.result);
//     }
//     swapchain_images_ = std::move(swapchain_images_r.value);
//
//     swapchain_format_ = surface_format.format;
//     swapchain_extent_ = swapchain_extent;
//
//     MLE_I("Swapchain created. Image count:{} Present mode:{} Surface format:[{}, {}] Extent:{}", swapchain_images_.size(),
//           vk::to_string(swapchain_ci.presentMode), vk::to_string(surface_format.format), vk::to_string(surface_format.colorSpace),
//           vec2u{swapchain_extent.width, swapchain_extent.height});
//
//     return Result::OK;
// }

// vk::CommandBuffer Impl::acquireCommandBuffer(CmdType type) {
//     auto& pool_data = getPool(type);
//
//     if (pool_data.buffers.empty()) {
//         vk::CommandBufferAllocateInfo cmd_alloc_info;
//         cmd_alloc_info.commandPool = pool_data.pool;
//         cmd_alloc_info.level = vk::CommandBufferLevel::ePrimary;
//         cmd_alloc_info.commandBufferCount = 1;
//         auto cmd_alloc_r = device_.allocateCommandBuffers(cmd_alloc_info);
//         if (cmd_alloc_r.result != vk::Result::eSuccess) {
//             core::unrecoverable("Failed to allocate command buffer! Error code: {}", cmd_alloc_r.result);
//         }
//         return cmd_alloc_r.value[0];
//     }
//
//     auto ret = pool_data.buffers.back();
//     pool_data.buffers.pop_back();
//     return ret;
// }
//
// void Impl::submitWait(vk::CommandBuffer cmd, CmdType type) {
//     auto fence = getFence();
//
//     vk::SubmitInfo submit_info;
//     submit_info.setCommandBuffers(cmd);
//
//     auto submit_r = getPool(type).queue.submit(submit_info, fence);
//     if (submit_r != vk::Result::eSuccess) {
//         core::unrecoverable("Failed to submit command buffer! Error code: {}", submit_r);
//     }
//
//     wait(fence, UINT64_MAX);
//     release(fence);
//
//     release(cmd, type);
// }
//
// void Impl::submitAsync(vk::CommandBuffer cmd, CmdType type, std::function<void(void)>&& callback) {
//     vk::SubmitInfo submit_info{};
//     submit_info.setCommandBuffers(cmd);
//
//     auto fence = getFence();
//
//     std::thread([this, cmd, type, fence, callback = std::move(callback)]() mutable {
//         wait(fence, UINT64_MAX);
//         release(fence);
//         callback();
//         release(cmd, type);
//     }).detach();
// }
}  // namespace

void init([[maybe_unused]] const CI& ci) {
    MLE_ASSERT(!i_);
    i_ = std::make_unique<Impl>();
    i_->init();
}

void shutdown() {
    if (i_) {
        MLE_I("Shutting down Renderer");
        i_->shutdown();
        i_.reset();
    }
}

void addOnShutdown(std::function<void(void)>&& func) {
    MLE_ASSERT(i_);
    i_->addOnShutdown(std::move(func));
}

namespace detail {
VkContext& getVk() {
    MLE_ASSERT(i_);
    return i_->getVk();
}

vk::Device getDevice() {
    MLE_ASSERT(i_);
    return i_->getDevice();
}

VmaAllocator getVma() {
    MLE_ASSERT(i_);
    return i_->getVma();
}

FencePool& getFencePool() {
    MLE_ASSERT(i_);
    return i_->getFencePool();
}
}  // namespace detail
}  // namespace mle::renderer
