#include "CommandManager.h"

#include "Renderer.h"
#include "mle/core/Assert.h"
#include "mle/core/Unwrap.h"

namespace mle {
CommandBuffer ResetCommandPool::getPrimary() {
    vk::CommandBuffer cmd;
    if (primary_index_ < available_primary_buffers_.size()) {
        cmd = available_primary_buffers_[primary_index_];
    } else {
        auto alloc_info = vk::CommandBufferAllocateInfo{};
        alloc_info.commandPool = o_;
        alloc_info.level = vk::CommandBufferLevel::ePrimary;
        alloc_info.commandBufferCount = 1;

        auto cmds = unwrap(Renderer::i().vk().getDevice().allocateCommandBuffers(alloc_info));
        MLE_ASSERT_LOG(cmds.size() == 1, "Expected to allocate exactly one command buffer, got: {}", cmds.size());
        cmd = cmds[0];
        available_primary_buffers_.push_back(cmd);
    }
    primary_index_++;
    check(cmd.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit}));
    return {cmd, queue_data_idx_, true};
}

CommandBuffer ResetCommandPool::getSecondary() {
    vk::CommandBuffer cmd;
    if (secondary_index_ < available_secondary_buffers_.size()) {
        cmd = available_secondary_buffers_[secondary_index_];
    } else {
        auto alloc_info = vk::CommandBufferAllocateInfo{};
        alloc_info.commandPool = o_;
        alloc_info.level = vk::CommandBufferLevel::eSecondary;
        alloc_info.commandBufferCount = 1;

        auto cmds = unwrap(Renderer::i().vk().getDevice().allocateCommandBuffers(alloc_info));
        MLE_ASSERT_LOG(cmds.size() == 1, "Expected to allocate exactly one command buffer, got: {}", cmds.size());
        cmd = cmds[0];
        available_secondary_buffers_.push_back(cmd);
    }
    secondary_index_++;
    check(cmd.begin({}));
    return {cmd, queue_data_idx_, false};
}

void ResetCommandPool::submit(CommandBuffer&& cmd, vk::SubmitInfo2 submit_info) {
    MLE_ASSERT_LOG(submit_info.commandBufferInfoCount == 1, "One, and only one, command buffer can be submitted here.");
    MLE_ASSERT(cmd.isPrimary());

    Renderer::i().cmdMgr().submit(queue_data_idx_, submit_info);

    reclaim(std::move(cmd));
}

// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved) enforcing ownership transfer
void ResetCommandPool::reclaim(CommandBuffer&& cmd) {
    if (cmd.isPrimary()) {
        available_primary_buffers_.push_back(cmd.get());
    } else {
        available_secondary_buffers_.push_back(cmd.get());
    }
}

void ResetCommandPool::reset() {
    Renderer::i().vk().getDevice().resetCommandPool(o_);
}

void RendererCommandManager::init() {
    const auto& queue_data = Renderer::i().vkCtx().getQueueData();

    if (queue_data.g_fam_idx != VkCtx::QueueData::INVALID_FAMILY) {
        cmd_type_to_index_[0] = 0;

        queue_data_[0].queue = queue_data.g_queue;
        queue_data_[0].family_index = queue_data.g_fam_idx;
    }
    if (queue_data.separate_compute) {
        cmd_type_to_index_[1] = 1;

        queue_data_[1].queue = queue_data.c_queue;
        queue_data_[1].family_index = queue_data.c_fam_idx;
    } else {
        cmd_type_to_index_[1] = 0;
    }
    if (queue_data.dedicated_transfer) {
        cmd_type_to_index_[2] = 2;

        queue_data_[2].queue = queue_data.t_queue;
        queue_data_[2].family_index = queue_data.t_fam_idx;
    } else {
        if (queue_data.g_fam_idx != VkCtx::QueueData::INVALID_FAMILY) {
            cmd_type_to_index_[2] = 0;
        } else {
            cmd_type_to_index_[2] = 1;
        }
    }
}

void RendererCommandManager::shutdown() {
    for (auto& q_data : queue_data_) {
        std::scoped_lock lock(q_data.pool_map_mutex);

        for (auto& [_, pool_data] : q_data.thread_ots_pool_map) {
            if (pool_data.pool) {
                Renderer::i().destroy(pool_data.pool);
            }
        }

        q_data.thread_ots_pool_map.clear();
    }
}

ResetCommandPool RendererCommandManager::makeResetableCommandPool(GCmdType type) {
    usize queue_idx = queueDataIdx(type);
    auto& q_data = queue_data_.at(queue_idx);

    auto pool_ci = vk::CommandPoolCreateInfo{};
    pool_ci.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    pool_ci.queueFamilyIndex = as<u32>(q_data.family_index);

    auto pool = unwrap(Renderer::i().vk().getDevice().createCommandPool(pool_ci));
    return {pool, queue_idx};
}

CommandBuffer RendererCommandManager::getOTS(usize queue_data_idx) {
    auto& q_data = queue_data_.at(queue_data_idx);

    vk::CommandBuffer cmd;

    {
        std::scoped_lock lock(q_data.pool_map_mutex);
        auto& ots_pool_data = q_data.thread_ots_pool_map[std::this_thread::get_id()];

        if (!ots_pool_data.pool) {
            auto pool_ci = vk::CommandPoolCreateInfo{};
            pool_ci.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
            pool_ci.queueFamilyIndex = as<u32>(q_data.family_index);
            ots_pool_data.pool = unwrap(Renderer::i().vk().getDevice().createCommandPool(pool_ci));
        }

        if (!ots_pool_data.available_primary_buffers.empty()) {
            cmd = ots_pool_data.available_primary_buffers.back();
            ots_pool_data.available_primary_buffers.pop_back();
        } else {
            auto alloc_info = vk::CommandBufferAllocateInfo{};
            alloc_info.commandPool = ots_pool_data.pool;
            alloc_info.level = vk::CommandBufferLevel::ePrimary;
            alloc_info.commandBufferCount = 1;

            auto cmds = unwrap(Renderer::i().vk().getDevice().allocateCommandBuffers(alloc_info));
            MLE_ASSERT_LOG(cmds.size() == 1, "Expected to allocate exactly one command buffer, got: {}", cmds.size());
            cmd = cmds[0];
        }
    }

    vk::CommandBufferBeginInfo begin_info{};
    begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    check(cmd.begin(begin_info));

    return {cmd, queue_data_idx, true};
}

Fence RendererCommandManager::submit(usize queue_data_idx, vk::SubmitInfo2 submit_info) {
    MLE_ASSERT_LOG(submit_info.commandBufferInfoCount == 1, "One, and only one, command buffer can be submitted here.");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    auto cmd = submit_info.pCommandBufferInfos[0].commandBuffer;

    check(cmd.end());

    auto& q_data = queue_data_.at(queue_data_idx);

    Fence fence = Renderer::i().syncMgr().acquireFence();
    {
        std::scoped_lock lock(q_data.submit_mutex);
        check(q_data.queue.submit2(submit_info, fence.get()));
    }
    return fence;
}

void RendererCommandManager::submitOTSWait(CommandBuffer&& cmd, vk::SubmitInfo2 submit_info) {
    if (submit_info.commandBufferInfoCount == 0) {
        vk::CommandBufferSubmitInfo command_buffer_info = cmd();
        submit_info.setCommandBufferInfos(command_buffer_info);
    }

    Fence fence = submit(cmd.queue_data_idx_, submit_info);

    auto tid = cmd.tid();
    MLE_ASSERT_LOG(tid == std::this_thread::get_id(), "Submitting thread must be the same as reclaiming thread.");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    MLE_ASSERT_LOG(submit_info.pCommandBufferInfos[0].commandBuffer == cmd.get(), "Submitted command buffer must be the same as the one provided.");

    check(fence.wait());
    reclaimOTS(std::move(cmd));
}

void RendererCommandManager::submitOTSAsync(CommandBuffer&& cmd, vk::SubmitInfo2 submit_info, std::move_only_function<void(void)>&& callback) {
    if (submit_info.commandBufferInfoCount == 0) {
        vk::CommandBufferSubmitInfo command_buffer_info = cmd();
        submit_info.setCommandBufferInfos(command_buffer_info);
    }

    Fence fence = submit(cmd.queue_data_idx_, submit_info);

    auto tid = cmd.tid();
    MLE_ASSERT_LOG(tid == std::this_thread::get_id(), "Submitting thread must be the same as reclaiming thread.");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    MLE_ASSERT_LOG(submit_info.pCommandBufferInfos[0].commandBuffer == cmd.get(), "Submitted command buffer must be the same as the one provided.");

    if (callback) {
        Renderer::i().syncMgr().trackFence(std::move(fence), [this, cmd = std::move(cmd), cb = std::move(callback)]() mutable {
            reclaimOTS(std::move(cmd));
            cb();
        });
    } else {
        Renderer::i().syncMgr().trackFence(std::move(fence), [this, cmd = std::move(cmd)]() mutable { reclaimOTS(std::move(cmd)); });
    }
}

// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved) enforcing ownership transfer
void RendererCommandManager::reclaimOTS(CommandBuffer&& cmd) {
    cmd().reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    auto& q_data = queue_data_.at(cmd.queue_data_idx_);
    std::scoped_lock lock(q_data.pool_map_mutex);
    auto& pool_data = q_data.thread_ots_pool_map[cmd.tid()];
    pool_data.available_primary_buffers.push_back(cmd());
}
}  // namespace mle
