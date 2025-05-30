#include "Renderer.h"

#include <vulkan/vk_enum_string_helper.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "mle/common/Exception.h"
#include "mle/common/Logger.h"
#include "mle/common/Utils.h"
#include "mle/core/Core.h"
#include "mle/renderer/Buffer.h"
#include "mle/renderer/Consts.h"
#include "mle/renderer/Events.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Utils.h"

namespace mle::renderer {
namespace {
struct FrameData {
    vk::Semaphore image_available_semaphore;
    vk::Semaphore render_finished_semaphore;
    vk::Fence render_finished_fence;
    vk::CommandPool command_pool;
    vk::CommandBuffer cmd;
    vk::QueryPool query_pool;

    std::vector<BufferHnd> buffers;
    std::vector<ImageHnd> images;

    std::vector<std::function<void(void)>> delete_queue;
};

struct Data {
    VkContext vk;
    ED ed;

    std::vector<ImageHnd> swapchain_images;

    usize frame_number = 0;
    usize current_frame_idx = 0;
    std::array<FrameData, 2> frames;
    usize current_swapchain_image_idx = max<usize>();

    bool swapchain_dirty = true;
    bool frame_active = false;

    vk::CommandPool transfer_ots_command_pool;
    vk::CommandBuffer transfer_ots_cmd;
    vk::Fence transfer_ots_fence;

    std::vector<std::function<void(void)>> shutdown_delete_queue;

    ImageHnd default_texture;

    bool at_render_pass = false;
    Recti current_render_area{};
    PipelineRef current_pipeline{};
};
std::unique_ptr<Data> d_;  // NOLINT

void createFramesData() {
    MLE_D("Creating frames data...");

    for (i32 i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        MLE_VT(i);
        auto& frame = d_->frames.at(i);

        MLE_T("    Sync objects...");

        vk::SemaphoreCreateInfo semaphore_ci{};

        frame.image_available_semaphore = d_->vk.getDevice().createSemaphore(semaphore_ci);
        frame.render_finished_semaphore = d_->vk.getDevice().createSemaphore(semaphore_ci);

        vk::FenceCreateInfo fence_ci{};
        fence_ci.flags = vk::FenceCreateFlagBits::eSignaled;
        frame.render_finished_fence = d_->vk.getDevice().createFence(fence_ci);

        MLE_T("    Command pool...");

        vk::CommandPoolCreateInfo command_pool_ci{};
        command_pool_ci.queueFamilyIndex = d_->vk.getAioQueueIdx();
        frame.command_pool = d_->vk.getDevice().createCommandPool(command_pool_ci);

        MLE_T("    Primary command buffer");
        vk::CommandBufferAllocateInfo command_buffer_ai{};
        command_buffer_ai.commandPool = frame.command_pool;
        command_buffer_ai.level = vk::CommandBufferLevel::ePrimary;
        command_buffer_ai.commandBufferCount = 1;
        frame.cmd = d_->vk.getDevice().allocateCommandBuffers(command_buffer_ai).at(0);

        MLE_T("    Query pool");
        vk::QueryPoolCreateInfo query_pool_ci;
        query_pool_ci.queryType = vk::QueryType::eTimestamp;
        query_pool_ci.queryCount = 2;
        frame.query_pool = d_->vk.getDevice().createQueryPool(query_pool_ci);
        d_->vk.getDevice().resetQueryPool(frame.query_pool, 0, 2);
    }
}

void createDefaultTexture() {
    MLE_D("Creating default texture...");
    Image::CI ci;
    ci.extent = {64, 64};
    ci.format = vk::Format::eR8G8B8A8Unorm;
    ci.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
    d_->default_texture = std::make_unique<Image>(ci);

    auto color1 = Color::MAGENTA.asABGR();
    auto color2 = Color::BLACK.asABGR();
    auto color3 = Color::BLACK.asABGR();
    auto color4 = Color::MAGENTA.asABGR();
    MLE_D("Default texture colors: {} {} {} {}", color1, color2, color3, color4);

    std::vector<u32> pixels(64UL * 64);
    for (usize i = 0; i < 32; i++) {
        for (usize j = 0; j < 32; j++) {
            pixels.at((i * 64) + j) = color1;
        }
    }
    for (usize i = 0; i < 32; i++) {
        for (usize j = 32; j < 64; j++) {
            pixels.at((i * 64) + j) = color2;
        }
    }
    for (usize i = 32; i < 64; i++) {
        for (usize j = 0; j < 32; j++) {
            pixels.at((i * 64) + j) = color3;
        }
    }
    for (usize i = 32; i < 64; i++) {
        for (usize j = 32; j < 64; j++) {
            pixels.at((i * 64) + j) = color4;
        }
    }

    auto ots = beginTransferOTS();
    auto buf = d_->default_texture->update(ots, pixels.data());
    d_->default_texture->transitionState(ots, Image::State::SHADER_READ);
    endTransferOTS();
}

FrameData& getCurrentFrame() {
    MLE_ASSERT(d_->frame_active);
    return d_->frames.at(d_->current_frame_idx);
}

ImageRef getCurrentSwapchainImage() {
    return d_->swapchain_images.at(d_->current_swapchain_image_idx).get();
}

bool isSwapchainVisible() {
    return d_->vk.isSwapchainVisible();
}

void createSwapchain() {
    MLE_I("Swapchain creation initialized...");

    waitIdle();

    auto old_format = d_->vk.getSwapchainFormat();

    d_->vk.createSwapchain();

    d_->swapchain_images.clear();
    d_->swapchain_images.resize(d_->vk.getSwapchainImageCount());
    for (usize i = 0; i < d_->vk.getSwapchainImageCount(); ++i) {
        Image::CISwapchain ci{d_->vk.getSwapchainImage(i)};
        d_->swapchain_images.at(i) = std::make_unique<Image>(ci);
    }

    events::SwapchainRecreated swapchain_recreated_event;
    swapchain_recreated_event.old_format = old_format;
    swapchain_recreated_event.new_format = d_->vk.getSwapchainFormat();
    d_->ed.dispatch(swapchain_recreated_event);

    d_->swapchain_dirty = false;

    MLE_I("Swapchain created successfully!");
}

void acquireNextImage() {
    try {
        auto acquire_next_image_rv =
            d_->vk.getDevice().acquireNextImageKHR(d_->vk.getSwapchain(), max<u64>(), d_->frames.at(d_->current_frame_idx).image_available_semaphore, nullptr);
        if (acquire_next_image_rv.result == vk::Result::eSuboptimalKHR) {
            d_->swapchain_dirty = true;
        }
        d_->current_swapchain_image_idx = acquire_next_image_rv.value;
    } catch (vk::OutOfDateKHRError&) {
        createSwapchain();
        acquireNextImage();
    }
}

}  // namespace

void init([[maybe_unused]] CI&& ci) {
    MLE_ASSERT(!d_);
    d_ = std::make_unique<Data>();

    MLE_I("Initializing Renderer");

    d_->vk.init();

    createFramesData();

    vk::CommandPoolCreateInfo transfer_command_pool_ci{};
    transfer_command_pool_ci.queueFamilyIndex = d_->vk.getTransferQueueIdx();
    transfer_command_pool_ci.flags = vk::CommandPoolCreateFlagBits::eTransient;
    d_->transfer_ots_command_pool = d_->vk.getDevice().createCommandPool(transfer_command_pool_ci);
    d_->transfer_ots_cmd = d_->vk.getDevice().allocateCommandBuffers({d_->transfer_ots_command_pool, vk::CommandBufferLevel::ePrimary, 1}).at(0);
    vk::FenceCreateInfo transfer_fence_ci{};
    d_->transfer_ots_fence = d_->vk.getDevice().createFence(transfer_fence_ci);

    createDefaultTexture();

    // FIXME: self.el_window_resize_ = Window::ed().makeEventListener<window::events::WindowResize>(nullptr, &Renderer::elfWindowResize);
    // self.el_window_iconify_ = Window::ed().makeEventListener<window::events::WindowIconify>(nullptr, &Renderer::elfWindowIconify);

    MLE_I("Module initialized successfully!");
}

// void Renderer::elfWindowResize([[maybe_unused]] void* user_ptr, [[maybe_unused]] const window::events::WindowResize& event) {
//     MLE_D("Window resize");
//     i().swapchain_dirty_ = true;
// }
//
// void Renderer::elfWindowIconify([[maybe_unused]] void* user_ptr, const window::events::WindowIconify& event) {
//     LOG_T("Window iconify {}", event.iconified);
//     i().iconified_ = event.iconified;
// }

void shutdown() {
    MLE_I("Shutting down Renderer Module");

    waitIdle();

    MLE_T("Executing shutdown delete queue...");
    for (auto& func : d_->shutdown_delete_queue) {
        func();
    }

    MLE_T("Destroying in flight frames...");
    for (auto& frame : d_->frames) {
        d_->vk.getDevice().destroy(frame.render_finished_fence);
        d_->vk.getDevice().destroy(frame.image_available_semaphore);
        d_->vk.getDevice().destroy(frame.render_finished_semaphore);
        d_->vk.getDevice().destroy(frame.command_pool);
        d_->vk.getDevice().destroy(frame.query_pool);
        frame.buffers.clear();
        frame.images.clear();
    }

    MLE_T("Destroying transfer OTS structures...");
    d_->vk.getDevice().destroy(d_->transfer_ots_command_pool);
    d_->vk.getDevice().destroy(d_->transfer_ots_fence);

    MLE_T("Destroying default texture...");
    d_->default_texture.reset();

    MLE_T("Destroying swapchain images...");
    d_->swapchain_images.clear();

    MLE_T("Shuting down Vulkan...");
    d_->vk.shutdown();

    d_.reset();
    MLE_I("Finished shutting down Renderer Module");
}

Result beginFrame() {
    d_->frame_number++;
    MLE_D("----- Begin frame {} -----", d_->frame_number);
    d_->current_frame_idx = d_->frame_number % FRAMES_IN_FLIGHT;

    auto& f = d_->frames.at(d_->current_frame_idx);

    auto fence_wait_result = d_->vk.getDevice().waitForFences(f.render_finished_fence, vk::True, DEFAULT_TIMEOUT);
    if (fence_wait_result != vk::Result::eSuccess) {
        // also throwing on timeout
        MLE_THROW(VK_ERROR, "Failed to wait for fence: {}", vk::to_string(fence_wait_result));
    }

    d_->vk.getDevice().resetFences(f.render_finished_fence);

    f.buffers.clear();
    f.images.clear();

    for (auto& func : f.delete_queue) {
        func();
    }
    f.delete_queue.clear();

    if (d_->swapchain_dirty) {
        createSwapchain();
    }

    acquireNextImage();

    if (!isSwapchainVisible()) {
        MLE_D("Swapchain not visible. Skipping frame.");
        return Result::FRAME_SKIP;
    }

    d_->frame_active = true;

    std::array<u64, 2> timestamps{};
    auto query_result = d_->vk.getDevice().getQueryPoolResults(f.query_pool, 0, 2, timestamps.size() * sizeof(u64), timestamps.data(), sizeof(u64),
                                                               vk::QueryResultFlagBits::e64);

    if (query_result == vk::Result::eSuccess || query_result == vk::Result::eNotReady) {
        auto elapsed = nanoseconds(static_cast<u64>(static_cast<f32>(timestamps.at(1) - timestamps.at(0)) * d_->vk.getTimestampPeriod()));
        core::addToSecondKPI(core::SecondKPIType::RENDERING, elapsed);
    } else {
        MLE_E("Failed to get query pool result: {}", vk::to_string(query_result));
    }

    d_->vk.getDevice().resetCommandPool(f.command_pool, {});

    vk::CommandBufferBeginInfo cmd_begin_info{};
    cmd_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    auto& cmd = f.cmd;
    cmd.begin(cmd_begin_info);

    cmd.resetQueryPool(f.query_pool, 0, 2);
    cmd.writeTimestamp2(vk::PipelineStageFlagBits2::eBottomOfPipe, f.query_pool, 0);

    return Result::OK;
}

void endFrame(ImageRef frame_image) {
    ImageRef swapchain_image = getCurrentSwapchainImage();
    auto& f = getCurrentFrame();
    auto& cmd = f.cmd;

    if (frame_image) {
        swapchain_image->updateBlit(cmd, frame_image);
    }
    swapchain_image->transitionState(cmd, Image::State::PRESENT);

    cmd.writeTimestamp2(vk::PipelineStageFlagBits2::eTopOfPipe, f.query_pool, 1);
    cmd.end();

    std::array<vk::PipelineStageFlags, 1> wait_stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

    vk::SubmitInfo submit_info{};
    submit_info.setCommandBuffers(cmd);
    submit_info.setWaitDstStageMask(wait_stages);
    submit_info.setWaitSemaphores(f.image_available_semaphore);
    submit_info.setSignalSemaphores(f.render_finished_semaphore);

    d_->vk.getAioQueue().submit(submit_info, f.render_finished_fence);

    std::array swapchains = {d_->vk.getSwapchain()};
    std::array image_indices = {static_cast<u32>(d_->current_swapchain_image_idx)};

    vk::PresentInfoKHR present_info{};
    present_info.setWaitSemaphores(f.render_finished_semaphore);
    present_info.setSwapchains(swapchains);
    present_info.setImageIndices(image_indices);

    try {
        auto present_result = d_->vk.getAioQueue().presentKHR(present_info);
        if (present_result == vk::Result::eSuboptimalKHR) {
            MLE_T("Swapchain suboptimal during present.");
            d_->swapchain_dirty = true;
        }
    } catch (vk::OutOfDateKHRError&) {
        MLE_I("Swapchain out of date during present.");
        d_->swapchain_dirty = true;
    } catch (vk::Error& e) {
        MLE_THROW(VK_ERROR, "Failed to present: {}", e.what());
    }

    d_->frame_active = false;
}

void beginRendering(RenderingInfo info) {
    auto cmd = getCurrentFrame().cmd;

    if (info.render_area.size.x == 0) {
        info.render_area.size.x = info.colors.at(0).image->getExtent().x - info.render_area.pos.x;
    }
    if (info.render_area.size.y == 0) {
        info.render_area.size.y = info.colors.at(0).image->getExtent().y - info.render_area.pos.y;
    }

    vk::RenderingInfo render_info;
    render_info.setRenderArea({
        {info.render_area.pos.x, info.render_area.pos.y},
        {static_cast<u32>(info.render_area.size.x), static_cast<u32>(info.render_area.size.y)},
    });
    render_info.setViewMask(0);
    render_info.setLayerCount(1);

    std::vector<vk::RenderingAttachmentInfo> attachment_descriptions{info.colors.size()};
    for (usize i = 0; i < info.colors.size(); i++) {
        auto& attachment = info.colors.at(i);
        auto& rattachment = attachment_descriptions.at(i);

        attachment.image->transitionState(cmd, Image::State::COLOR_ATT);

        if (attachment.view) {
            rattachment.imageView = attachment.view;
        } else {
            rattachment.imageView = attachment.image->getDefaultView();
        }
        rattachment.loadOp = attachment.load;
        rattachment.clearValue = attachment.clear_value;
        rattachment.storeOp = attachment.store;
        rattachment.imageLayout = vk::ImageLayout::eAttachmentOptimal;
    }
    render_info.setColorAttachments(attachment_descriptions);

    vk::RenderingAttachmentInfo depth_attachment;
    if (info.depth.image || info.depth.view) {
        if (!info.depth.view) {
            info.depth.view = info.depth.image->getDefaultView();
        }

        info.depth.image->transitionState(cmd, Image::State::DEPTH_ATT);

        depth_attachment.imageView = info.depth.view;
        depth_attachment.loadOp = info.depth.load;
        depth_attachment.clearValue = info.depth.clear_value;
        depth_attachment.storeOp = info.depth.store;
        depth_attachment.imageLayout = vk::ImageLayout::eAttachmentOptimal;

        render_info.setPDepthAttachment(&depth_attachment);
    }

    cmd.beginRendering(render_info);
    MLE_ASSERT(!d_->at_render_pass);
    d_->at_render_pass = true;
    d_->current_render_area = info.render_area;
}

void endRendering() {
    if (d_->at_render_pass) {
        getCurrentFrame().cmd.endRendering();
        d_->current_pipeline = nullptr;
        d_->current_render_area = {};
        d_->at_render_pass = false;
    }
}

void bindGraphicsPipeline(PipelineRef pipeline) {
    if (d_->current_pipeline == pipeline) {
        return;
    }
    d_->current_pipeline = pipeline;
    getFrameMainCommandBuffer().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->getPipeline());
}

void waitIdle() {
    MLE_I("WaitIdle");
    d_->vk.waitIdle();
}

ED& getED() {
    return d_->ed;
}

VkContext& getVk() {
    return d_->vk;
}

usize getFrameNumber() {
    return d_->frame_number;
}

usize getFrameId() {
    return d_->current_frame_idx;
}

ShaderModuleRef getShaderModule(const fs::path& path) {
    return ShaderModule::get(path);
}

PipelineGetResult getPipeline(const std::string& name) {
    return Pipeline::get(name);
}

vk::CommandBuffer getFrameMainCommandBuffer() {
    return getCurrentFrame().cmd;
}

void addToFrameDeleteQueue(std::function<void(void)>&& func) {
    getCurrentFrame().delete_queue.push_back(std::move(func));
}

void addToFrameDeleteQueue(BufferHnd&& hnd) {
    getCurrentFrame().buffers.push_back(std::move(hnd));
}

void addToFrameDeleteQueue(ImageHnd&& hnd) {
    getCurrentFrame().images.push_back(std::move(hnd));
}

BufferRef createBufferForFrame(const Buffer::CI& ci) {
    return getCurrentFrame().buffers.emplace_back(std::make_unique<Buffer>(ci)).get();
}

vk::CommandBuffer beginTransferOTS() {
    d_->transfer_ots_cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    return d_->transfer_ots_cmd;
}

void endTransferOTS() {
    d_->transfer_ots_cmd.end();
    vk::SubmitInfo submit_info;
    submit_info.setCommandBuffers(d_->transfer_ots_cmd);
    d_->vk.getTransferQueue().submit(submit_info, d_->transfer_ots_fence);
    auto fence_r = d_->vk.getDevice().waitForFences(d_->transfer_ots_fence, vk::True, DEFAULT_TIMEOUT);
    if (fence_r != vk::Result::eSuccess) {
        MLE_THROW(VK_ERROR, "Failed to wait for fence: {}", vk::to_string(fence_r));
    }
    d_->vk.getDevice().resetFences(d_->transfer_ots_fence);
    d_->vk.getDevice().resetCommandPool(d_->transfer_ots_command_pool, {});
}

bool isFrameActive() {
    return d_->frame_active;
}

ImageRef getDefaultTextureImage() {
    return d_->default_texture.get();
}

vk::Format getDefaultColorFormat() {
    return d_->vk.getColorFormat();
}

vk::Format getDepthFormat() {
    return d_->vk.getDepthFormat();
}

Recti getCurrentRenderArea() {
    return d_->current_render_area;
}

void addToShutdownDeleteQueue(std::function<void(void)>&& func) {
    d_->shutdown_delete_queue.push_back(std::move(func));
}
}  // namespace mle::renderer
