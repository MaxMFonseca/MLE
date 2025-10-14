#include "FrameRenderer.h"

#include <fmt/ranges.h>

#include <atomic>
#include <ranges>
#include <utility>

#include "CommandManager.h"
#include "Renderer.h"
#include "mle/client/Client.h"
#include "mle/core/PerfTracker.h"
#include "mle/core/RuntimeConfig.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"
#include "mle/window/Window.h"
#include "vulkan/vulkan.hpp"

namespace mle {
void FrameRenderer::init() {
    MLE_I("Initializing FrameRenderer");

    initFramesData();

    swapchain_rtcl0_ = RuntimeConfig::i().listen("renderer.swapchain.present_mode", [this]() { swapchain_dirty_.store(true, std::memory_order_relaxed); });
    swapchain_rtcl1_ = RuntimeConfig::i().listen("renderer.swapchain.triple_buffer", [this]() { swapchain_dirty_.store(true, std::memory_order_relaxed); });

    default_clear_color_rtcl_ = RuntimeConfig::i().listen("renderer.default_clear_color", [this]() {
        auto color_str = RuntimeConfig::i().getString("renderer.default_clear_color", "BLACK");
        auto color = Color::fromString(color_str);
        default_clear_color_ = toVkColor(color);
    });
    default_clear_color_ = toVkColor(Color::fromString(RuntimeConfig::i().getString("renderer.default_clear_color", "BLACK")));

    target_fps_.store(RuntimeConfig::i().getUInt("renderer.target_fps", max<u32>()), std::memory_order_relaxed);

    window_resize_listener_ = Window::i().getED().makeListener<window::ev::Resize>([this](const window::ev::Resize& ev) {
        MLE_I("Window resize event received: {}x{}", ev.size.x, ev.size.y);
        swapchain_dirty_.store(true, std::memory_order_relaxed);
    });

    auto force_swapchain_result = createSwapchain();
    MLE_ASSERT(force_swapchain_result == Result::OK);

    running_.store(true, std::memory_order_relaxed);
    run_thread_ = std::jthread([this](std::stop_token st) { runLoop(std::move(st)); });
    MLE_I("FrameRenderer run thread started.");
    MLE_D("FrameRenderer initialized successfully!");
}

// NOLINTNEXTLINE(performance-unnecessary-value-param) stop_token is small and cheap to copy
void FrameRenderer::runLoop(std::stop_token st) {
    while (!st.stop_requested()) {
        Stopwatch sw;

        MLE_PERF_SCOPE("FrameRenderer::runLoop");

        const auto br = beginFrame();
        if (br == Result::FRAME_SKIP || br == Result::SWAPCHAIN_NOT_VISIBLE) {
            std::this_thread::sleep_for(50ms);
            continue;
        }
        if (isError(br)) {
            Core::i().unrecoverable("beginFrame() failed with result: {}", as<Result>(br));
            break;
        }

        ImageRef frame_img = Client::i().render();

        endFrame(frame_img);

        const u32 target = target_fps_.load(std::memory_order_relaxed);
        if (target != max<u32>() && target > 0) {
            const auto budget_ns = std::chrono::nanoseconds(1'000'000'000ULL / target);
            const auto spent = sw.elapsed<std::chrono::nanoseconds>();
            if (spent < budget_ns) {
                std::this_thread::sleep_for(budget_ns - spent);
            }
        }
    }

    Renderer::i().commandManager().waitIdle(GCmdType::G);

    running_.store(false, std::memory_order_relaxed);
    MLE_I("FrameRenderer run loop exited.");
}

void FrameRenderer::stopRun() {
    if (!isRunning()) {
        return;
    }

    MLE_I("Stopping FrameRenderer run thread...");
    run_thread_.request_stop();
    std::jthread old = std::move(run_thread_);
    running_.store(false, std::memory_order_relaxed);
    MLE_I("FrameRenderer run thread stopped.");
}

void FrameRenderer::shutdown() {
    MLE_I("shutdown");

    stopRun();

    auto device = Renderer::i().vkDevice();

    if (swapchain_) {
        MLE_I("Destroying swapchain...");
        device.destroy(swapchain_);
        swapchain_ = nullptr;
        for (auto s : swapchain_image_semaphores_) {
            Renderer::i().destroy(s);
        }
    }
    swapchain_images_.clear();
    for (auto& f : frames_) {
        f.delete_images.clear();
        f.delete_buffers.clear();
        for (auto& it : std::ranges::reverse_view(f.delete_stack)) {
            it();
        }
        device.destroy(f.query_pool);
        device.destroy(f.image_available_semaphore);
        device.destroy(f.render_finished_fence);
        f.cmd_pool.shutdown();
    }
}

void FrameRenderer::initFramesData() {
    MLE_D("Creating frames data...");

    auto device = Renderer::i().vkDevice();

    for (i32 i = 0; i < 2; ++i) {
        MLE_VT(i);
        auto& frame = frames_.at(i);

        MLE_T("    Sync objects");

        frame.image_available_semaphore = unwrap(device.createSemaphore({}));
        frame.render_finished_fence = unwrap(device.createFence({vk::FenceCreateFlagBits::eSignaled}));

        MLE_T("    Query pool");
        vk::QueryPoolCreateInfo query_pool_ci;
        query_pool_ci.queryType = vk::QueryType::eTimestamp;
        query_pool_ci.queryCount = 2;
        frame.query_pool = unwrap(device.createQueryPool(query_pool_ci));
        device.resetQueryPool(frame.query_pool, 0, 2);

        MLE_T("    Command pool");
        frame.cmd_pool = Renderer::i().commandManager().createResetCommandPool(GCmdType::G);
    }
}

namespace {
vk::SurfaceFormatKHR chooseSwapchainSurfaceFormat(const auto& p_device) {
    const auto available_formats = unwrap(p_device.getSurfaceFormatsKHR(Renderer::i().vk().getSurface()));
    std::array target_formats = {vk::Format::eB8G8R8A8Srgb, vk::Format::eR8G8B8A8Srgb, vk::Format::eB8G8R8A8Unorm, vk::Format::eR8G8B8A8Unorm};

    for (const auto& t : target_formats) {
        for (const auto avail : available_formats) {
            if (avail.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear && avail.format == t) {
                return avail;
            }
        }
    }
    return available_formats[0];
}

vk::PresentModeKHR chooseSwapchainPresentMode(const auto& p_device) {
    const auto available_present_modes = unwrap(p_device.getSurfacePresentModesKHR(Renderer::i().vk().getSurface()));

    vk::PresentModeKHR target_present_mode{};

    std::string target_present_mode_str = RuntimeConfig::i().getString("renderer.swapchain.present_mode", "auto");
    if (target_present_mode_str == "mailbox") {
        target_present_mode = vk::PresentModeKHR::eMailbox;
    } else if (target_present_mode_str == "immediate") {
        target_present_mode = vk::PresentModeKHR::eImmediate;
    } else {
        if (target_present_mode_str != "auto") {
            MLE_W("Unknown present mode '{}', falling back to 'auto' (FIFO).", target_present_mode_str);
        }
        return vk::PresentModeKHR::eFifo;
    }

    for (const auto& avail : available_present_modes) {
        if (avail == target_present_mode) {
            return avail;
        }
    }

    MLE_W("Requested present mode {} is not available, falling back to FIFO.", vk::to_string(target_present_mode));
    return vk::PresentModeKHR::eFifo;
}

u32 chooseSwapchainImageCount(const auto& p_device) {
    const auto surface_capabilities = unwrap(p_device.getSurfaceCapabilitiesKHR(Renderer::i().vk().getSurface()));

    u32 target_img_count = 3;
    bool target_triple_buffer = RuntimeConfig::i().getBool("renderer.swapchain.triple_buffer", true);
    if (!target_triple_buffer) {
        target_img_count = 2;
    }

    return std::clamp(target_img_count, surface_capabilities.minImageCount,
                      surface_capabilities.maxImageCount == 0 ? target_img_count : surface_capabilities.maxImageCount);
}
}  // namespace

Result FrameRenderer::createSwapchain() {
    MLE_I("Swapchain creation initialized...");

    auto g_lock = Renderer::i().commandManager().waitIdle(GCmdType::G);

    auto old_swapchain = swapchain_;
    // auto old_format = surface_format_;

    auto p_device = Renderer::i().vk().getPhysicalDevice().o;
    auto device = Renderer::i().vkDevice();
    auto surface = Renderer::i().vk().getSurface();

    auto surface_capabilities = unwrap(p_device.getSurfaceCapabilitiesKHR(surface));
    swapchain_extent_ = {surface_capabilities.currentExtent.width, surface_capabilities.currentExtent.height};

    if (swapchain_extent_.x == 0xFFFFFFFF || swapchain_extent_.y == 0xFFFFFFFF || swapchain_extent_.x == 0 || swapchain_extent_.y == 0) {
        MLE_I("Surface capabilities has zero or undefined extent. Checking window state...");
        auto window_size = Window::i().getSize();
        if (window_size.x == 0 || window_size.y == 0) {
            MLE_I("Window is iconified, cannot create swapchain.");
            iconified_ = true;
            swapchain_dirty_.store(true, std::memory_order_relaxed);
            swapchain_visible_ = false;
            return Result::SWAPCHAIN_NOT_VISIBLE;
        }

        swapchain_extent_ = window_size;
    }

    surface_format_ = chooseSwapchainSurfaceFormat(p_device);
    present_mode_ = chooseSwapchainPresentMode(p_device);

    vk::SwapchainCreateInfoKHR swapchain_ci{};
    swapchain_ci.surface = surface;
    swapchain_ci.minImageCount = chooseSwapchainImageCount(p_device);
    swapchain_ci.imageFormat = surface_format_.format;
    swapchain_ci.imageColorSpace = surface_format_.colorSpace;
    swapchain_ci.imageExtent = toVkExtent2D(swapchain_extent_);
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.imageUsage = swapchain_usage_;
    swapchain_ci.preTransform = surface_capabilities.currentTransform;
    swapchain_ci.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchain_ci.presentMode = present_mode_;
    swapchain_ci.clipped = vk::True;
    swapchain_ci.oldSwapchain = old_swapchain;
    swapchain_ci.imageSharingMode = vk::SharingMode::eExclusive;
    swapchain_ci.queueFamilyIndexCount = 0;
    swapchain_ci.pQueueFamilyIndices = nullptr;

    swapchain_ = unwrap(device.createSwapchainKHR(swapchain_ci));

    if (old_swapchain) {
        swapchain_images_.clear();
        device.destroy(old_swapchain);
        for (auto sem : swapchain_image_semaphores_) {
            device.destroy(sem);
        }
        swapchain_image_semaphores_.clear();
    }

    Image::CI image_ci;
    image_ci.extent = swapchain_extent_;
    image_ci.format = Image::Format::SWAPCHAIN;

    auto raw_swapchain_images = unwrap(device.getSwapchainImagesKHR(swapchain_));
    swapchain_images_.resize(raw_swapchain_images.size());
    for (usize i = 0; i < raw_swapchain_images.size(); ++i) {
        image_ci.non_owned_image = raw_swapchain_images.at(i);
        swapchain_images_.at(i) = Image::createHnd(image_ci);
        swapchain_image_semaphores_.push_back(unwrap(device.createSemaphore({})));
    }

    swapchain_dirty_.store(false, std::memory_order_relaxed);
    swapchain_visible_ = true;

    MLE_I("Swapchain created successfully!");

    // FIXME: dispatch swapchain recreated event

    return Result::OK;
}

void FrameRenderer::logSwapchainInfo() {
    MLE_I("Logging swapchain info...");
    MLE_I("    Swapchain: {}", (void*)swapchain_);
    MLE_I("    Image count: {}", swapchain_images_.size());
    MLE_I("    Surface format: {}, {}", to_string(surface_format_.format), to_string(surface_format_.colorSpace));
    MLE_I("    Present mode: {}", to_string(present_mode_));
    MLE_I("    Extent: {}x{}", swapchain_extent_.x, swapchain_extent_.y);
}

Result FrameRenderer::beginFrame() {
    if (iconified_) {
        MLE_I("Window is iconified, skipping frame.");
        return Result::FRAME_SKIP;
    }

    MLE_ASSERT(current_frame_ == NO_FRAME);

    MLE_T("beginFrame. NextFrame: {}", next_frame_);

    auto device = Renderer ::i().vkDevice();

    auto& next_frame = getNextFrame();

    check(device.waitForFences(next_frame.render_finished_fence, vk::True, DEFAULT_TIMEOUT_NS));
    check(device.resetFences(next_frame.render_finished_fence));

    MLE_T("Cleaning up next frame resources...");

    next_frame.delete_buffers.clear();
    next_frame.delete_images.clear();
    next_frame.host_visible_buffers.clear();

    for (auto& it : std::ranges::reverse_view(next_frame.delete_stack)) {
        it();
    }
    next_frame.delete_stack.clear();

    std::array<u64, 2> timestamps{};
    auto query_result =
        device.getQueryPoolResults(next_frame.query_pool, 0, 2, timestamps.size() * sizeof(u64), timestamps.data(), sizeof(u64), vk::QueryResultFlagBits::e64);
    if (query_result == vk::Result::eSuccess || query_result == vk::Result::eNotReady) {
        auto elapsed = std::chrono::nanoseconds(as<u64>(as<f32>(timestamps.at(1) - timestamps.at(0)) * Renderer::i().vk().getTimestampPeriod()));
        static const PerfTracker::CounterId FRAME_TIME_COUNTER_ID = PerfTracker::i().registerCounter("Frame Time");
        PerfTracker::i().record(FRAME_TIME_COUNTER_ID, elapsed);
        MLE_T("Frame GPU time: {} ns", elapsed.count());
    } else {
        MLE_E("Failed to get query pool result: {}", vk::to_string(query_result));
    }

    next_frame.cmd_pool.reset();

    if (swapchain_dirty_.load(std::memory_order_relaxed)) {
        auto create_result = createSwapchain();
        if (create_result == Result::SWAPCHAIN_NOT_VISIBLE) {
            MLE_I("Swapchain not visible, skipping frame.");
            return Result::FRAME_SKIP;
        }
        if (isError(create_result)) {
            Core::i().unrecoverable("Failed to create swapchain: {}", as<Result>(create_result));
        }
    }

    auto acquire_next_image_rv = acquireNextImage();
    if (!acquire_next_image_rv) {
        return acquire_next_image_rv.error();
    }

    current_frame_ = next_frame_;
    next_frame_ = (next_frame_ + 1) % 2;
    current_swapchain_image_idx_ = acquire_next_image_rv.value();
    current_swapchain_image_ = swapchain_images_.at(current_swapchain_image_idx_).get();

    MLE_T("Frame setup successful. Frame index: {}, Swapchain image index: {}", current_frame_, current_swapchain_image_idx_);

    auto& f = getCurrentFrame();

    current_primary_cmd_ = f.cmd_pool.getPrimary();

    cmd().resetQueryPool(f.query_pool, 0, 2);
    cmd().writeTimestamp2(vk::PipelineStageFlagBits2::eBottomOfPipe, f.query_pool, 0);

    return Result::OK;
}

void FrameRenderer::endFrame(ImageRef frame_image) {
    auto& f = getCurrentFrame();
    auto& cmd = current_primary_cmd_;

    if (frame_image) {
        current_swapchain_image_->blitImage(cmd, *frame_image);
    } else {
        current_swapchain_image_->transitionState(cmd, Image::State::TRANSFER_DST);
        vk::ImageSubresourceRange subresource_range{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
        cmd().clearColorImage(current_swapchain_image_->get(), vk::ImageLayout::eTransferDstOptimal, default_clear_color_, subresource_range);
    }
    current_swapchain_image_->transitionState(cmd, Image::State::PRESENT);

    cmd().writeTimestamp2(vk::PipelineStageFlagBits2::eTopOfPipe, f.query_pool, 1);

    vk::CommandBuffer command_buffer = cmd();
    vk::CommandBufferSubmitInfo command_buffer_info{};
    command_buffer_info.commandBuffer = command_buffer;
    command_buffer_info.deviceMask = 0;

    vk::SemaphoreSubmitInfo wait_semaphore_info{};
    wait_semaphore_info.semaphore = f.image_available_semaphore;
    wait_semaphore_info.stageMask = vk::PipelineStageFlagBits2::eAllCommands;

    auto swapchain_image_semaphore = swapchain_image_semaphores_.at(current_swapchain_image_idx_);

    vk::SemaphoreSubmitInfo signal_semaphore_info{};
    signal_semaphore_info.semaphore = swapchain_image_semaphore;
    signal_semaphore_info.stageMask = vk::PipelineStageFlagBits2::eAllCommands;

    vk::SubmitInfo2 submit_info{};
    submit_info.setCommandBufferInfos(command_buffer_info);
    submit_info.setWaitSemaphoreInfos(wait_semaphore_info);
    submit_info.setSignalSemaphoreInfos(signal_semaphore_info);

    f.cmd_pool.submit(std::move(cmd), submit_info, f.render_finished_fence);

    std::array swapchains = {swapchain_};
    std::array image_indices = {current_swapchain_image_idx_};

    vk::PresentInfoKHR present_info{};
    present_info.setWaitSemaphores(swapchain_image_semaphore);
    present_info.setSwapchains(swapchains);
    present_info.setImageIndices(image_indices);

    auto present_result = Renderer::i().cmdMgr().submitPresent(present_info);
    if (present_result == vk::Result::eSuboptimalKHR || present_result == vk::Result::eErrorOutOfDateKHR) {
        MLE_I("Swapchain NOK durint present: {}, setting dirty.", present_result);
        swapchain_dirty_.store(true, std::memory_order_relaxed);
    } else if (present_result != vk::Result::eSuccess) {
        check(present_result);
    }

    MLE_T("Frame {}({}) submited successfully to swapchain image index: {}", current_frame_, frames_finished_, current_swapchain_image_idx_);

    current_frame_ = NO_FRAME;
    frames_finished_++;
    current_swapchain_image_ = nullptr;
    current_swapchain_image_idx_ = max<u32>();
}

void FrameRenderer::recreateNextFrameImageAvailableSemaphore() {
    MLE_VC(0);
    auto& next_frame = getNextFrame();
    Renderer::i().destroy(next_frame.image_available_semaphore);
    vk::SemaphoreCreateInfo semaphore_ci{};
    next_frame.image_available_semaphore = unwrap(Renderer::i().vkDevice().createSemaphore(semaphore_ci));
}

Expected<u32> FrameRenderer::acquireNextImage() {
    auto& next_frame = getNextFrame();

    int iter_limit = 300;
    while (iter_limit > 0) {
        iter_limit--;

        auto acquire_next_image_rv = Renderer::i().vkDevice().acquireNextImageKHR(swapchain_, max<u64>(), next_frame.image_available_semaphore, nullptr);
        if (acquire_next_image_rv.result == vk::Result::eSuccess) {
            return acquire_next_image_rv.value;
        }
        if (acquire_next_image_rv.result == vk::Result::eErrorOutOfDateKHR || acquire_next_image_rv.result == vk::Result::eSuboptimalKHR) {
            MLE_I("Swapchain is out of date or suboptimal, recreating...");
            recreateNextFrameImageAvailableSemaphore();

            if (createSwapchain() == Result::SWAPCHAIN_NOT_VISIBLE) {
                return std::unexpected(Result::SWAPCHAIN_NOT_VISIBLE);
            }
            std::this_thread::sleep_for(20ms);
        } else {
            check(acquire_next_image_rv.result);
        }
    }

    Core::i().unrecoverable("Failed to acquire next image after 300 attempts.");
    return std::unexpected(Result::NOK);
}

CommandBuffer FrameRenderer::getSecondaryCommandBuffer() {
    assertInFrame();
    return getCurrentFrame().cmd_pool.getSecondary();
};

BufferSlice FrameRenderer::getHostVisibleBuffer(usize size, vk::BufferUsageFlags usage) {
    usage |= vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
    auto& f = getCurrentFrame();
    auto& buffers = f.host_visible_buffers[usage];

    usize alignment = Renderer::i().vk().getAlignmentForBufferUsage(usage);

    for (auto& buffer : buffers) {
        if (buffer.second + size <= buffer.first->getSize()) {
            BufferSlice ret{.buffer = buffer.first.get(), .size = size, .offset = buffer.second};
            buffer.second = alignUp(buffer.second + size, alignment);
            return ret;
        }
    }

    Buffer::CI buffer_ci{};
    buffer_ci.size = std::max(size, min_host_visible_buffer_size_);
    buffer_ci.usage = usage;
    buffer_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY_HOST_WRITE_SEQ;
    auto& er = buffers.emplace_back(Buffer::createHnd(buffer_ci), alignUp(size, alignment));
    return BufferSlice{.buffer = er.first.get(), .size = size, .offset = 0};
}

void FrameRenderer::deleteAfterFrame(BufferHnd&& buffer) {
    if (inFrame()) {
        getCurrentFrame().delete_buffers.emplace_back(std::move(buffer));
    } else {
        getNextFrame().delete_buffers.emplace_back(std::move(buffer));
    }
}

void FrameRenderer::deleteAfterFrame(ImageHnd&& image) {
    if (inFrame()) {
        getCurrentFrame().delete_images.emplace_back(std::move(image));
    } else {
        getNextFrame().delete_images.emplace_back(std::move(image));
    }
}

void FrameRenderer::addToFrameDeleteStack(std::move_only_function<void(void)>&& func) {
    if (inFrame()) {
        getCurrentFrame().delete_stack.emplace_back(std::move(func));
    } else {
        getNextFrame().delete_stack.emplace_back(std::move(func));
    }
}
}  // namespace mle
