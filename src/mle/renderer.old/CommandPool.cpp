#include "CommandPool.h"

#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "Fence.h"
#include "Renderer.h"
#include "detail/CommandPoolManager.h"
#include "mle/common/Assert.h"
#include "mle/renderer/Utils.h"

namespace mle::renderer {
void CommandPool::reset() {
    if (type_ == CmdType::INVALID) {
        return;
    }

    MLE_ASSERT_LOG(cmd_buffer_count_ == o_.available_buffers.size(),
                   "A cmd buffer wasnt submited or is pending before pool deletion. Dangling cmd buffers! Count: {}, Available: {}", cmd_buffer_count_,
                   o_.available_buffers.size());

    detail::getCommandPoolManager().release(type_, std::move(o_));
    type_ = CmdType::INVALID;
}

CommandPool::CommandPool(CommandPool&& other) :
    type_(other.type_),
    o_(std::move(other.o_)) {
    other.type_ = CmdType::INVALID;
}

CommandPool::~CommandPool() {
    reset();
}

void CommandPool::init(CmdType type) {
    type_ = type;
    o_ = detail::getCommandPoolManager().acquirePool(type);
    cmd_buffer_count_ = o_.available_buffers.size();
}

void CommandPool::releaseCmdBuffer(vk::CommandBuffer cmd) {
    std::scoped_lock lock(mutex_);
    o_.available_buffers.emplace_back(cmd);
}

void CommandPool::submitWait(vk::CommandBuffer cmd) {
    check(cmd.end());

    vk::CommandBufferSubmitInfo cmd_info;
    cmd_info.setCommandBuffer(cmd);

    vk::SubmitInfo2 info;
    info.setCommandBufferInfos(cmd_info);

    auto fence = detail::getCommandPoolManager().submit(type_, info);
    fence.wait();

    releaseCmdBuffer(cmd);
}

void CommandPool::submitAsync(vk::SubmitInfo2 submit_info, std::move_only_function<void(void)>&& callback) {
    auto cmd = submit_info.pCommandBufferInfos[0].commandBuffer;  // NOLINT

    check(cmd.end());

    auto fence = detail::getCommandPoolManager().submit(type_, submit_info);

    fence.waitAsync([callback = std::move(callback), this, cmd]() mutable {
        releaseCmdBuffer(cmd);
        if (callback) {
            callback();
        }
    });
}

void CommandPool::submitAsync(vk::CommandBuffer cmd, std::move_only_function<void(void)>&& callback) {
    vk::CommandBufferSubmitInfo cmd_info;
    cmd_info.setCommandBuffer(cmd);

    vk::SubmitInfo2 info;
    info.setCommandBufferInfos(cmd_info);

    submitAsync(info, std::move(callback));
}

vk::CommandBuffer CommandPool::getCmd() {
    // THis makes no sense, right?
    // A pool has to be created per thread, so locking here is just dumb
    std::scoped_lock lock(mutex_);

    vk::CommandBuffer cmd;

    if (!o_.available_buffers.empty()) {
        cmd = o_.available_buffers.back();
        o_.available_buffers.pop_back();
    } else {
        vk::CommandBufferAllocateInfo alloc_info;
        alloc_info.setCommandPool(o_.o).setLevel(vk::CommandBufferLevel::ePrimary).setCommandBufferCount(1);
        auto alloc_r = detail::getDevice().allocateCommandBuffers(alloc_info);
        check(alloc_r.result);
        cmd_buffer_count_++;
        cmd = alloc_r.value[0];
    }

    vk::CommandBufferBeginInfo bi{};
    bi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    check(cmd.begin(bi));
    return cmd;
}
}  // namespace mle::renderer
