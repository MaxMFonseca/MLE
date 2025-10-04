#pragma once

#include <map>

#include "mle/renderer/Buffer.h"
#include "mle/renderer/Consts.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Types.h"
#include "mle/window/Events.h"

namespace mle::renderer::detail {
struct FrameData {
    vk::Semaphore image_available_semaphore;
    vk::Semaphore render_finished_semaphore;
    vk::Fence render_finished_fence;
    vk::CommandPool command_pool;
    vk::CommandBuffer cmd;
    std::vector<vk::CommandBuffer> secondary_cmds;
    usize secondary_cmds_idx = 0;
    vk::QueryPool query_pool;

    std::vector<vk::DescriptorPool> descriptor_pools;  ///< Descriptor pools for this frame, used to allocate descriptor sets.

    std::vector<std::function<void(void)>> delete_stack;  ///< Functions to call at the end of the frame to clean up resources.
    std::vector<BufferHnd> delete_buffers{};              ///< Buffers to delete at the end of the frame.
    std::vector<ImageHnd> delete_images{};                ///< Images to delete at the end of the frame.
    std::map<vk::BufferUsageFlags, std::vector<std::pair<BufferHnd, usize>>> host_visible_buffers;  ///< Host-visible buffers allocated for this frame.
};

class FrameRenderer final {
  public:
    FrameRenderer(const FrameRenderer&) = delete;
    FrameRenderer& operator=(const FrameRenderer&) = delete;
    FrameRenderer(FrameRenderer&&) = delete;
    FrameRenderer& operator=(FrameRenderer&&) = delete;

    FrameRenderer() = default;
    ~FrameRenderer() = default;

    void init();
    void reset();

    Result beginFrame();
    void endFrame(ImageRef frame_image);
    void submitJob(vk::CommandBuffer cmd);

    vk::CommandBuffer getCmd();
    vk::CommandBuffer getCmdSecondary();

    void addToCallOnFrameEnd(std::function<void(void)>&& func);
    void deleteAfterFrame(BufferHnd&& buffer);
    void deleteAfterFrame(ImageHnd&& image);

    BufferSlice getHostVisibleBuffer(usize size, vk::BufferUsageFlags usage);

    [[nodiscard]] bool inFrame() const { return current_frame_ != NO_FRAME; }

  private:
    void initFramesData();
    Expected<usize> acquireNextImage();

    Result createSwapchain();
    void recreateNextFrameImageAvailableSemaphore();

    FrameData& getCurrentFrame();
    FrameData& getNextFrame();

    void iconifyCb(bool iconified) { iconified_ = iconified; }

  private:
    vk::SwapchainKHR swapchain_;
    vk::Extent2D swapchain_extent_;
    vk::Format swapchain_format_ = vk::Format::eUndefined;

    std::vector<ImageHnd> swapchain_images_;
    ImageRef current_swapchain_image_ = nullptr;
    usize current_swapchain_image_idx_ = max<usize>();

    vk::Queue render_queue_;

    std::array<FrameData, FRAMES_IN_FLIGHT> frames_{};

    int current_frame_ = NO_FRAME;
    int next_frame_ = 0;
    bool swapchain_dirty_ = true;
    bool swapchain_visible_ = false;
    bool iconified_ = false;

    window::WindowIconifyListener window_iconify_listener_;
};
}  // namespace mle::renderer::detail
