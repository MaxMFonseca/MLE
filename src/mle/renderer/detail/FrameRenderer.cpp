#include "FrameRenderer.h"

#include <ranges>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "VkContext.h"
#include "mle/core/Core.h"
#include "mle/renderer/Consts.h"
#include "mle/renderer/Utils.h"
#include "mle/window/Window.h"

namespace mle::renderer::detail {
void FrameRenderer::init() {
    MLE_I("Initializing FrameRenderer");

    initFramesData();

    render_queue_ = getDevice().getQueue(getVk().getQueueIndex(CmdType::GRAPHICS), 0);

    window_iconify_listener_ =
        window::getED().makeEventListener<window::events::WindowIconify>([this](const window::events::WindowIconify& event) { iconifyCb(event.iconified); });

    MLE_D("FrameRenderer initialized successfully!");
}

void FrameRenderer::reset() {
    MLE_I("shutdown");

    if (swapchain_) {
        MLE_I("Destroying swapchain...");
        getDevice().destroy(swapchain_);
        swapchain_ = nullptr;
    }
    swapchain_images_.clear();
    for (auto& f : frames_) {
        f.delete_images.clear();
        f.delete_buffers.clear();
        for (auto& it : std::ranges::reverse_view(f.delete_stack)) {
            it();
        }
        getDevice().destroy(f.query_pool);
        getDevice().destroy(f.command_pool);
        getDevice().destroy(f.image_available_semaphore);
        getDevice().destroy(f.render_finished_semaphore);
        getDevice().destroy(f.render_finished_fence);
    }
    window_iconify_listener_.reset();
}

void FrameRenderer::initFramesData() {
    MLE_D("Creating frames data...");

    for (i32 i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        MLE_VT(i);
        auto& frame = frames_.at(i);

        MLE_T("    Sync objects...");

        vk::SemaphoreCreateInfo semaphore_ci{};

        auto s1_r = getDevice().createSemaphore(semaphore_ci);
        if (s1_r.result != vk::Result::eSuccess) {
            core::unrecoverable("Failed to create image available semaphore: {}", as<vk::Result>(s1_r.result));
        }
        frame.image_available_semaphore = s1_r.value;
        auto s2_r = getDevice().createSemaphore(semaphore_ci);
        if (s2_r.result != vk::Result::eSuccess) {
            core::unrecoverable("Failed to create render finished semaphore: {}", as<vk::Result>(s2_r.result));
        }
        frame.render_finished_semaphore = s2_r.value;

        vk::FenceCreateInfo fence_ci{};
        fence_ci.flags = vk::FenceCreateFlagBits::eSignaled;
        auto fence_r = getDevice().createFence(fence_ci);
        if (fence_r.result != vk::Result::eSuccess) {
            core::unrecoverable("Failed to create render finished fence: {}", as<vk::Result>(fence_r.result));
        }
        frame.render_finished_fence = fence_r.value;

        MLE_T("    Command pool...");

        vk::CommandPoolCreateInfo command_pool_ci{};
        command_pool_ci.queueFamilyIndex = getVk().getQueueIndex(CmdType::GRAPHICS);
        auto command_pool_r = getDevice().createCommandPool(command_pool_ci);
        if (command_pool_r.result != vk::Result::eSuccess) {
            core::unrecoverable("Failed to create command pool: {}", as<vk::Result>(command_pool_r.result));
        }
        frame.command_pool = command_pool_r.value;

        MLE_T("    Render command buffer");
        vk::CommandBufferAllocateInfo command_buffer_ai{};
        command_buffer_ai.commandPool = frame.command_pool;
        command_buffer_ai.level = vk::CommandBufferLevel::ePrimary;
        command_buffer_ai.commandBufferCount = 1;
        auto command_buffer_r = getDevice().allocateCommandBuffers(command_buffer_ai);
        if (command_buffer_r.result != vk::Result::eSuccess) {
            core::unrecoverable("Failed to allocate command buffer: {}", as<vk::Result>(command_buffer_r.result));
        }
        frame.cmd = command_buffer_r.value.at(0);

        MLE_T("    Query pool");
        vk::QueryPoolCreateInfo query_pool_ci;
        query_pool_ci.queryType = vk::QueryType::eTimestamp;
        query_pool_ci.queryCount = 2;
        auto query_pool_r = getDevice().createQueryPool(query_pool_ci);
        if (query_pool_r.result != vk::Result::eSuccess) {
            core::unrecoverable("Failed to create query pool: {}", as<vk::Result>(query_pool_r.result));
        }
        frame.query_pool = query_pool_r.value;
        getDevice().resetQueryPool(frame.query_pool, 0, 2);
    }
}

namespace {
vk::SurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats, vk::SurfaceFormatKHR target_format) {
    for (const auto& format : available_formats) {
        if (format.format == target_format.format && format.colorSpace == target_format.colorSpace) {
            return format;
        }
    }

    return available_formats[0];
}
}  // namespace

Result FrameRenderer::createSwapchain() {
    MLE_C("Swapchain creation initialized...");

    waitIdle();

    auto old_swapchain = swapchain_;
    auto old_format = swapchain_format_;

    auto p_device = getVk().getPhysicalDevice();
    auto device = getDevice();
    auto surface = getVk().getSurface();

    auto surface_capabilities = unwrap(p_device.getSurfaceCapabilitiesKHR(surface));
    auto surface_present_modes = unwrap(p_device.getSurfacePresentModesKHR(surface));
    auto surface_formats = unwrap(p_device.getSurfaceFormatsKHR(surface));

    auto swapchain_extent = surface_capabilities.currentExtent;

    if (swapchain_extent.width == 0xFFFFFFFF || swapchain_extent.height == 0xFFFFFFFF || swapchain_extent.width == 0 || swapchain_extent.height == 0) {
        MLE_I("Swapchain extent is: {}. Returning swapchain not visible.", vec2u{swapchain_extent.width, swapchain_extent.height});
        swapchain_dirty_ = true;
        swapchain_visible_ = false;
        return Result::SWAPCHAIN_NOT_VISIBLE;
    }

    auto surface_format = chooseSwapchainSurfaceFormat(surface_formats, vk::SurfaceFormatKHR{getVk().getColorFormat(), vk::ColorSpaceKHR::eSrgbNonlinear});

#ifdef MLE_RUNTIME_CONFIG_PRESENT_MODE
    auto surface_present_mode = runtime_config::getPresentMode();
#else
    auto surface_present_mode = vk::PresentModeKHR::eFifo;
#endif

    u32 image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) {
        image_count = surface_capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapchain_ci{};
    swapchain_ci.surface = surface;
    swapchain_ci.minImageCount = image_count;
    swapchain_ci.imageFormat = surface_format.format;
    swapchain_ci.imageColorSpace = surface_format.colorSpace;
    swapchain_ci.imageExtent = swapchain_extent;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.imageUsage = vk::ImageUsageFlagBits::eTransferDst;
    swapchain_ci.preTransform = surface_capabilities.currentTransform;
    swapchain_ci.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchain_ci.presentMode = surface_present_mode;
    swapchain_ci.clipped = vk::True;
    swapchain_ci.oldSwapchain = old_swapchain;
    swapchain_ci.imageSharingMode = vk::SharingMode::eExclusive;
    swapchain_ci.queueFamilyIndexCount = 0;
    swapchain_ci.pQueueFamilyIndices = nullptr;

    swapchain_ = unwrap(device.createSwapchainKHR(swapchain_ci));

    if (old_swapchain) {
        swapchain_images_.clear();
        device.destroy(old_swapchain);
    }

    Image::CI image_ci;
    image_ci.format = swapchain_ci.imageFormat;
    image_ci.extent = {swapchain_ci.imageExtent.width, swapchain_ci.imageExtent.height};
    image_ci.usage = swapchain_ci.imageUsage;

    auto raw_swapchain_images = unwrap(device.getSwapchainImagesKHR(swapchain_));
    swapchain_images_.resize(raw_swapchain_images.size());
    for (usize i = 0; i < raw_swapchain_images.size(); ++i) {
        image_ci.o = raw_swapchain_images.at(i);
        swapchain_images_.at(i) = Image::createHnd(image_ci);
    }

    swapchain_format_ = surface_format.format;
    swapchain_extent_ = swapchain_extent;

    MLE_I("Swapchain info: Image count:{} Present mode:{} Surface format:[{}, {}] Extent:{}", swapchain_images_.size(), vk::to_string(swapchain_ci.presentMode),
          vk::to_string(surface_format.format), vk::to_string(surface_format.colorSpace), vec2u{swapchain_extent.width, swapchain_extent.height});

    events::SwapchainRecreated swapchain_recreated_event;
    swapchain_recreated_event.old_format = old_format;
    swapchain_recreated_event.new_format = swapchain_format_;
    getED().dispatch(swapchain_recreated_event);

    swapchain_dirty_ = false;
    swapchain_visible_ = true;

    MLE_I("Swapchain created successfully!");

    return Result::OK;
}

FrameData& FrameRenderer::getCurrentFrame() {
    MLE_ASSERT(current_frame_ != NO_FRAME);
    return frames_.at(current_frame_);
}

FrameData& FrameRenderer::getNextFrame() {
    return frames_.at(next_frame_);
}

void FrameRenderer::recreateNextFrameImageAvailableSemaphore() {
    auto& next_frame = getNextFrame();
    getDevice().destroy(next_frame.image_available_semaphore);
    vk::SemaphoreCreateInfo semaphore_ci{};
    next_frame.image_available_semaphore = unwrap(getDevice().createSemaphore(semaphore_ci));
}

Expected<usize> FrameRenderer::acquireNextImage() {
    auto& next_frame = getNextFrame();

    int iter_limit = 1000;
    while (iter_limit > 0) {
        iter_limit--;

        auto acquire_next_image_rv = getDevice().acquireNextImageKHR(swapchain_, max<u64>(), next_frame.image_available_semaphore, nullptr);
        if (acquire_next_image_rv.result == vk::Result::eSuccess) {
            return acquire_next_image_rv.value;
        }
        if (acquire_next_image_rv.result == vk::Result::eErrorOutOfDateKHR || acquire_next_image_rv.result == vk::Result::eSuboptimalKHR) {
            MLE_I("Swapchain is out of date or suboptimal, recreating...");
            recreateNextFrameImageAvailableSemaphore();

            if (createSwapchain() == Result::SWAPCHAIN_NOT_VISIBLE) {
                return std::unexpected(Result::SWAPCHAIN_NOT_VISIBLE);
            }
        } else {
            // unhandled errors
            check(acquire_next_image_rv.result);
        }
    }

    core::unrecoverable("Failed to acquire next image after 1000 attempts.");
    return std::unexpected(Result::NOK);
}

Result FrameRenderer::beginFrame() {
    if (iconified_) {
        MLE_I("Window is iconified, skipping frame.");
        return Result::FRAME_SKIP;
    }

    MLE_ASSERT_LOG(current_frame_ == NO_FRAME, "Cannot begin frame when a frame is already in progress. Call endFrame() first.");

    MLE_D("beginFrame. NextFrame: {}", next_frame_);

    auto device = getDevice();

    auto& next_frame = getNextFrame();

    auto fence_wait_result = device.waitForFences(next_frame.render_finished_fence, vk::True, DEFAULT_TIMEOUT_NS);
    check(fence_wait_result);

    device.resetFences(next_frame.render_finished_fence);

    next_frame.delete_buffers.clear();
    next_frame.delete_images.clear();
    for (auto& it : std::ranges::reverse_view(next_frame.delete_stack)) {
        it();
    }
    next_frame.delete_stack.clear();

    if (swapchain_dirty_) {
        auto create_result = createSwapchain();
        if (create_result == Result::SWAPCHAIN_NOT_VISIBLE) {
            MLE_I("Swapchain not visible, skipping frame.");
            return Result::FRAME_SKIP;
        }
        if (isError(create_result)) {
            core::unrecoverable("Failed to create swapchain: {}", as<Result>(create_result));
        }
    }

    auto acquire_next_image_rv = acquireNextImage();
    if (!acquire_next_image_rv) {
        return acquire_next_image_rv.error();
    }

    current_frame_ = next_frame_;
    next_frame_ = (next_frame_ + 1) % FRAMES_IN_FLIGHT;
    current_swapchain_image_idx_ = acquire_next_image_rv.value();
    current_swapchain_image_ = swapchain_images_.at(current_swapchain_image_idx_).get();

    MLE_T("Frame setup successful. Frame index: {}, Swapchain image index: {}", current_frame_, current_swapchain_image_idx_);

    auto& f = getCurrentFrame();

    std::array<u64, 2> timestamps{};
    auto query_result =
        device.getQueryPoolResults(f.query_pool, 0, 2, timestamps.size() * sizeof(u64), timestamps.data(), sizeof(u64), vk::QueryResultFlagBits::e64);

    if (query_result == vk::Result::eSuccess || query_result == vk::Result::eNotReady) {
        auto elapsed = std::chrono::nanoseconds(static_cast<u64>(static_cast<f32>(timestamps.at(1) - timestamps.at(0)) * getVk().getTimestampPeriod()));
        core::accumulateKPI(core::SecondKPIType::RENDERING, elapsed);
    } else {
        MLE_E("Failed to get query pool result: {}", vk::to_string(query_result));
    }

    device.resetCommandPool(f.command_pool, {});

    vk::CommandBufferBeginInfo cmd_begin_info{};
    cmd_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    auto& cmd = f.cmd;
    check(cmd.begin(cmd_begin_info));

    cmd.resetQueryPool(f.query_pool, 0, 2);
    cmd.writeTimestamp2(vk::PipelineStageFlagBits2::eBottomOfPipe, f.query_pool, 0);

    return Result::OK;
}

void FrameRenderer::endFrame(ImageRef frame_image) {
    auto& f = getCurrentFrame();
    auto& cmd = f.cmd;

    if (frame_image) {
        current_swapchain_image_->updateBlit(cmd, frame_image);
    } else {
        current_swapchain_image_->transitionState(cmd, Image::State::TRANSFER_DST);
        vk::ImageSubresourceRange subresource_range{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
        vk::ClearColorValue color;
        color.setFloat32({0.1F, 0.1F, 0.1F, 1.0F});
        cmd.clearColorImage(current_swapchain_image_->get(), vk::ImageLayout::eTransferDstOptimal, color, subresource_range);
    }
    current_swapchain_image_->transitionState(cmd, Image::State::PRESENT);

    cmd.writeTimestamp2(vk::PipelineStageFlagBits2::eTopOfPipe, f.query_pool, 1);
    check(cmd.end());

    std::array<vk::PipelineStageFlags, 1> wait_stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

    vk::SubmitInfo submit_info{};
    submit_info.setCommandBuffers(cmd);
    submit_info.setWaitDstStageMask(wait_stages);
    submit_info.setWaitSemaphores(f.image_available_semaphore);
    submit_info.setSignalSemaphores(f.render_finished_semaphore);

    check(render_queue_.submit(submit_info, f.render_finished_fence));

    std::array swapchains = {swapchain_};
    std::array image_indices = {static_cast<u32>(current_swapchain_image_idx_)};

    vk::PresentInfoKHR present_info{};
    present_info.setWaitSemaphores(f.render_finished_semaphore);
    present_info.setSwapchains(swapchains);
    present_info.setImageIndices(image_indices);

    auto present_result = render_queue_.presentKHR(present_info);
    if (present_result == vk::Result::eSuboptimalKHR || present_result == vk::Result::eErrorOutOfDateKHR) {
        MLE_I("Swapchain NOK durint present: {}, setting dirty.", present_result);
        swapchain_dirty_ = true;
    } else if (present_result != vk::Result::eSuccess) {
        check(present_result);
    }

    MLE_T("Frame {} ended successfully. Swapchain image index: {}", current_frame_, current_swapchain_image_idx_);

    current_frame_ = NO_FRAME;
    current_swapchain_image_ = nullptr;
    current_swapchain_image_idx_ = max<usize>();
}
}  // namespace mle::renderer::detail
