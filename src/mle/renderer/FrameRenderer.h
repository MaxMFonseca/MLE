#pragma once

#include "Buffer.h"
#include "CommandManager.h"
#include "Image.h"
#include "Types.h"
#include "Utils.h"
#include "mle/core/RuntimeConfig.h"
#include "mle/utils/Color.h"
#include "mle/window/Events.h"
#include "vulkan/vulkan.hpp"

namespace mle {
class FrameRenderer final {
  public:
    static constexpr usize NO_FRAME = max<usize>();

  private:
    struct FrameData {
        ResetCommandPool cmd_pool;

        vk::Semaphore image_available_semaphore;
        vk::Fence render_finished_fence;
        vk::QueryPool query_pool;

        std::vector<std::move_only_function<void(void)>> delete_stack;
        std::vector<BufferHnd> delete_buffers{};
        std::vector<ImageHnd> delete_images{};
        std::map<vk::BufferUsageFlags, std::vector<std::pair<BufferHnd, usize>>> host_visible_buffers;
    };

  public:
    MLE_NO_COPY_MOVE(FrameRenderer);

    ~FrameRenderer();

    [[nodiscard]] bool inFrame() const { return current_frame_ != NO_FRAME; }
    void assertInFrame() const { MLE_ASSERT_LOG(inFrame(), "Not in frame!"); }

    [[nodiscard]] auto getSwapchainFormat() const { return surface_format_.format; }
    [[nodiscard]] auto getSwapchianImageUsage() const { return swapchain_usage_; }
    [[nodiscard]] auto getSwapchainExtent() const { return swapchain_extent_; }
    [[nodiscard]] auto getCurrentFrameId() const { return current_frame_; }
    [[nodiscard]] auto getDefaultClearColor() const { return default_clear_color_; }

    [[nodiscard]] vk::CommandBuffer cmd() const {
        assertInFrame();
        return current_primary_cmd_();
    }
    CommandBuffer getSecondaryCommandBuffer();

    BufferSlice getHostVisibleBuffer(usize size, vk::BufferUsageFlags usage);

    void addToFrameDeleteStack(std::move_only_function<void(void)>&& func);

    void deleteAfterFrame(BufferHnd&& buffer);
    void deleteAfterFrame(ImageHnd&& image);

    [[nodiscard]] bool isRunning() const { return running_.load(std::memory_order_relaxed); }

  private:
    friend class Renderer;
    FrameRenderer() = default;

    void init();
    void shutdown();
    void initFramesData();

    void stopRun();
    void runLoop(std::stop_token st);

    Result beginFrame();
    void endFrame(ImageRef frame_image);

    void recreateNextFrameImageAvailableSemaphore();
    Expected<u32> acquireNextImage();

    Result createSwapchain();

    void logSwapchainInfo();

    FrameData& getCurrentFrame() {
        assertInFrame();
        return frames_.at(current_frame_);
    }
    FrameData& getNextFrame() { return frames_.at(next_frame_); }

  private:
    vk::SwapchainKHR swapchain_{};
    vec2u swapchain_extent_{};
    vk::SurfaceFormatKHR surface_format_ = {};
    vk::ImageUsageFlags swapchain_usage_ = vk::ImageUsageFlagBits::eTransferDst;
    std::vector<ImageHnd> swapchain_images_{};
    std::vector<vk::Semaphore> swapchain_image_semaphores_{};
    vk::PresentModeKHR present_mode_ = vk::PresentModeKHR::eFifo;
    ImageRef current_swapchain_image_ = nullptr;
    u32 current_swapchain_image_idx_ = max<u32>();
    CommandBuffer current_primary_cmd_{};
    bool swapchain_visible_ = false;
    bool iconified_ = false;

    vk::ClearColorValue default_clear_color_ = toVkColor(Color::BLACK);

    std::array<FrameData, 2> frames_;
    usize current_frame_ = NO_FRAME;
    usize next_frame_ = 0;
    usize frames_finished_ = 0;

    usize min_host_visible_buffer_size_ = 256UL * 1024;

    std::atomic<bool> swapchain_dirty_ = true;

    std::jthread run_thread_{};
    std::atomic<u32> target_fps_ = max<u32>();
    std::atomic<bool> running_{false};

    RuntimeConfigListenerHnd swapchain_rtcl0_;
    RuntimeConfigListenerHnd swapchain_rtcl1_;
    RuntimeConfigListenerHnd default_clear_color_rtcl_{};
    RuntimeConfigListenerHnd target_fps_rtcl_{};

    window::ev::ResizeL window_resize_listener_;
};
}  // namespace mle
