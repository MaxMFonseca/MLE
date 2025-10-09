#include "CommandManager.h"

#include "Renderer.h"
#include "mle/core/Assert.h"
#include "mle/core/Unwrap.h"

namespace mle {
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

RendererCommandManager::QueueData& RendererCommandManager::queueData(GCmdType type) {
    auto qd_idx = cmd_type_to_index_.at(as<usize>(type));
    if (qd_idx == max<usize>()) {
        MLE_UNREACHABLE_LOG("Requested command queue for unsupported queue type: {}", type);
    }
    return queue_data_.at(qd_idx);
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
    for (auto p : external_pools_) {
        Renderer::i().destroy(p);
    }
}

vk::CommandPool RendererCommandManager::makeCmdPool(GCmdType type, vk::CommandPoolCreateFlags flags) {
    auto& q_data = queueData(type);

    auto pool_ci = vk::CommandPoolCreateInfo{};
    pool_ci.flags = flags;
    pool_ci.queueFamilyIndex = as<u32>(q_data.family_index);

    auto pool = unwrap(Renderer::i().vk().getDevice().createCommandPool(pool_ci));
    external_pools_.push_back(pool);
    return pool;
}

vk::CommandBuffer RendererCommandManager::getOTS(GCmdType type) {
    auto& q_data = queueData(type);

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

        if (!ots_pool_data.available_buffers.empty()) {
            cmd = ots_pool_data.available_buffers.back();
            ots_pool_data.available_buffers.pop_back();
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

    return cmd;
}

Fence RendererCommandManager::submit(GCmdType type, vk::SubmitInfo2 submit_info) {
    MLE_ASSERT_LOG(submit_info.commandBufferInfoCount == 1, "One, and only one, command buffer can be submitted here.");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    auto cmd = submit_info.pCommandBufferInfos[0].commandBuffer;

    check(cmd.end());

    auto& q_data = queueData(type);

    Fence fence = Renderer::i().syncMgr().acquireFence();
    {
        std::scoped_lock lock(q_data.submit_mutex);
        check(q_data.queue.submit2(submit_info, fence.get()));
    }
    return fence;
}

void RendererCommandManager::submitOTSWait(GCmdType type, vk::SubmitInfo2 submit_info) {
    Fence fence = submit(type, submit_info);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    auto cmd = submit_info.pCommandBufferInfos[0].commandBuffer;
    auto tid = std::this_thread::get_id();
    check(fence.wait());
    reclaimOTS(cmd, type, tid);
}

void RendererCommandManager::submitOTSAsync(GCmdType type, vk::SubmitInfo2 submit_info, std::move_only_function<void(void)>&& callback) {
    Fence fence = submit(type, submit_info);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    auto cmd = submit_info.pCommandBufferInfos[0].commandBuffer;
    auto tid = std::this_thread::get_id();
    Renderer::i().syncMgr().trackFence(std::move(fence), [this, cmd, cmd_type = type, tid, cb = std::move(callback)]() mutable {
        reclaimOTS(cmd, cmd_type, tid);
        cb();
    });
}

void RendererCommandManager::reclaimOTS(vk::CommandBuffer cmd, GCmdType type, std::thread::id tid) {
    cmd.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    auto& q_data = queueData(type);
    std::scoped_lock lock(q_data.pool_map_mutex);
    auto& pool_data = q_data.thread_ots_pool_map[tid];
    pool_data.available_buffers.push_back(cmd);
}
}  // namespace mle
