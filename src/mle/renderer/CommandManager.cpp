#include "CommandManager.h"

#include "Renderer.h"
#include "mle/core/Assert.h"
#include "mle/core/Logger.h"
#include "mle/core/Unwrap.h"

namespace mle {
CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other) {
    MLE_ASSERT_LOG(o_ == nullptr, "Trying to move to a CommandBuffer that already owns a Vulkan command buffer. Submit/Reclaim it first.");
    if (this != &other) {
        o_ = other.o_;
        tid_ = other.tid_;
        queue_data_idx_ = other.queue_data_idx_;
        primary_ = other.primary_;
        other.o_ = nullptr;
    }
    return *this;
}

CommandBuffer::CommandBuffer(CommandBuffer&& other) :
    tid_(other.tid_),
    queue_data_idx_(other.queue_data_idx_),
    primary_(other.primary_) {
    MLE_ASSERT_LOG(o_ == nullptr, "Trying to move to a CommandBuffer that already owns a Vulkan command buffer. Submit/Reclaim it first.");
    o_ = other.o_;
    other.o_ = nullptr;
}

ResetCommandPool& ResetCommandPool::operator=(ResetCommandPool&& other) {
    MLE_ASSERT_LOG(o_ == nullptr, "Trying to move to a ResetCommandPool that already owns a Vulkan command pool. Shutdown it first.");
    if (this != &other) {
        o_ = other.o_;
        other.o_ = nullptr;
        queue_data_idx_ = other.queue_data_idx_;
        primary_buffers_ = std::move(other.primary_buffers_);
        secondary_buffers_ = std::move(other.secondary_buffers_);
    }
    return *this;
}

ResetCommandPool::ResetCommandPool(ResetCommandPool&& other) :
    queue_data_idx_(other.queue_data_idx_),
    primary_buffers_(std::move(other.primary_buffers_)),
    secondary_buffers_(std::move(other.secondary_buffers_)) {
    MLE_ASSERT_LOG(o_ == nullptr, "Trying to move to a ResetCommandPool that already owns a Vulkan command pool. Shutdown it first.");
    o_ = other.o_;
    other.o_ = nullptr;
}

void ResetCommandPool::shutdown() {
    if (o_) {
        MLE_T("Destroying ResetCommandPool {}", (void*)o_);
        Renderer::i().destroy(o_);
        o_ = nullptr;
    }
}

CommandBuffer ResetCommandPool::getPrimary() {
    vk::CommandBuffer cmd;
    if (primary_index_ == primary_buffers_.size()) {
        auto alloc_info = vk::CommandBufferAllocateInfo{};
        alloc_info.commandPool = o_;
        alloc_info.level = vk::CommandBufferLevel::ePrimary;
        alloc_info.commandBufferCount = 1;

        auto cmds = unwrap(Renderer::i().vk().getDevice().allocateCommandBuffers(alloc_info));
        MLE_ASSERT_LOG(cmds.size() == 1, "Expected to allocate exactly one command buffer, got: {}", cmds.size());
        primary_buffers_.push_back(cmds[0]);
    }
    cmd = primary_buffers_[primary_index_];
    primary_index_++;
    check(cmd.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit}));
    return {cmd, queue_data_idx_, true};
}

CommandBuffer ResetCommandPool::getSecondary() {
    vk::CommandBuffer cmd;
    if (secondary_index_ == secondary_buffers_.size()) {
        auto alloc_info = vk::CommandBufferAllocateInfo{};
        alloc_info.commandPool = o_;
        alloc_info.level = vk::CommandBufferLevel::eSecondary;
        alloc_info.commandBufferCount = 1;

        auto cmds = unwrap(Renderer::i().vk().getDevice().allocateCommandBuffers(alloc_info));
        MLE_ASSERT_LOG(cmds.size() == 1, "Expected to allocate exactly one command buffer, got: {}", cmds.size());
        secondary_buffers_.push_back(cmds[0]);
    }
    cmd = secondary_buffers_[secondary_index_];
    secondary_index_++;
    vk::CommandBufferInheritanceInfo inheritance_info{};
    vk::CommandBufferBeginInfo begin_info{};
    begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    begin_info.setPInheritanceInfo(&inheritance_info);
    check(cmd.begin(begin_info));
    return {cmd, queue_data_idx_, false};
}

// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved) intentional
void ResetCommandPool::submitWait(CommandBuffer&& cmd, vk::SubmitInfo2 submit_info) const {
    MLE_ASSERT_LOG(submit_info.commandBufferInfoCount == 1, "One, and only one, command buffer can be submitted here.");
    MLE_ASSERT(cmd.isPrimary());

    check(Renderer::i().cmdMgr().submit(queue_data_idx_, submit_info).wait());

    cmd.o_ = nullptr;
}

// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved) intentional
void ResetCommandPool::submit(CommandBuffer&& cmd, vk::SubmitInfo2 submit_info, vk::Fence fence) const {
    MLE_ASSERT_LOG(submit_info.commandBufferInfoCount == 1, "One, and only one, command buffer can be submitted here.");
    MLE_ASSERT(cmd.isPrimary());

    Renderer::i().cmdMgr().submit(queue_data_idx_, submit_info, fence);

    cmd.o_ = nullptr;
}

void ResetCommandPool::reset() {
    check(Renderer::i().vk().getDevice().resetCommandPool(o_));
    primary_index_ = 0;
    secondary_index_ = 0;
}

void RendererCommandManager::init() {
    const auto& queue_data = Renderer::i().vkCtx().getQueueData();

    if (queue_data.g_fam_idx != VkCtx::QueueData::INVALID_FAMILY) {
        cmd_type_to_qdidx_[0] = 0;

        queue_data_[0].queue = queue_data.g_queue;
        queue_data_[0].family_index = queue_data.g_fam_idx;
    }
    if (queue_data.separate_compute) {
        cmd_type_to_qdidx_[1] = 1;

        queue_data_[1].queue = queue_data.c_queue;
        queue_data_[1].family_index = queue_data.c_fam_idx;
    } else {
        cmd_type_to_qdidx_[1] = 0;
    }
    if (queue_data.dedicated_transfer) {
        cmd_type_to_qdidx_[2] = 2;

        queue_data_[2].queue = queue_data.t_queue;
        queue_data_[2].family_index = queue_data.t_fam_idx;
    } else {
        if (queue_data.g_fam_idx != VkCtx::QueueData::INVALID_FAMILY) {
            cmd_type_to_qdidx_[2] = 0;
        } else {
            cmd_type_to_qdidx_[2] = 1;
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

ResetCommandPool RendererCommandManager::createResetCommandPool(GCmdType type) {
    QueueDataIdx qdidx = queueDataIdx(type);
    auto& q_data = queue_data_.at(qdidx);

    auto pool_ci = vk::CommandPoolCreateInfo{};
    pool_ci.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    pool_ci.queueFamilyIndex = as<u32>(q_data.family_index);

    auto pool = unwrap(Renderer::i().vk().getDevice().createCommandPool(pool_ci));
    return {pool, qdidx};
}

CommandBuffer RendererCommandManager::getOTS(QueueDataIdx queue_data_idx) {
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
            std::ignore = cmd.reset();
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

void RendererCommandManager::submit(QueueDataIdx queue_data_idx, vk::SubmitInfo2 submit_info, vk::Fence fence) {
    MLE_ASSERT_LOG(submit_info.commandBufferInfoCount == 1, "One, and only one, command buffer can be submitted here.");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    auto cmd = submit_info.pCommandBufferInfos[0].commandBuffer;

    check(cmd.end());

    auto& q_data = queue_data_.at(queue_data_idx);

    {
        std::scoped_lock lock(q_data.submit_mutex);
        check(q_data.queue.submit2(submit_info, fence));
    }
}

Fence RendererCommandManager::submit(QueueDataIdx queue_data_idx, vk::SubmitInfo2 submit_info) {
    Fence fence = Renderer::i().syncMgr().acquireFence();
    submit(queue_data_idx, submit_info, fence.get());
    return fence;
}

void RendererCommandManager::submitOTSWait(CommandBuffer&& cmd, vk::SubmitInfo2 submit_info) {
    vk::CommandBufferSubmitInfo command_buffer_info = cmd();
    if (submit_info.commandBufferInfoCount == 0) {
        submit_info.setCommandBufferInfos(command_buffer_info);
    }

    Fence fence = submit(cmd.queue_data_idx_, submit_info);

    auto tid = cmd.tid();
    MLE_ASSERT_LOG(tid == std::this_thread::get_id(), "Submitting thread must be the same as eeclaiming thread.");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    MLE_ASSERT_LOG(submit_info.pCommandBufferInfos[0].commandBuffer == cmd.get(), "Submitted command buffer must be the same as the one provided.");

    check(fence.wait());
    reclaimOTS(std::move(cmd));
}

void RendererCommandManager::submitOTSAsync(CommandBuffer&& cmd, vk::SubmitInfo2 submit_info, std::move_only_function<void(void)>&& callback) {
    vk::CommandBufferSubmitInfo command_buffer_info = cmd();
    if (submit_info.commandBufferInfoCount == 0) {
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
    auto& q_data = queue_data_.at(cmd.queue_data_idx_);
    std::scoped_lock lock(q_data.pool_map_mutex);
    auto& pool_data = q_data.thread_ots_pool_map[cmd.tid()];
    pool_data.available_primary_buffers.push_back(cmd());
    cmd.o_ = nullptr;
}

std::unique_lock<std::mutex> RendererCommandManager::waitIdle(GCmdType type) {
    MLE_D("waitIdle queue {}", type);
    auto& q_data = queue_data_.at(queueDataIdx(type));
    std::unique_lock<std::mutex> lock(q_data.submit_mutex);
    check(q_data.queue.waitIdle());
    return lock;
}

void RendererCommandManager::waitDeviceIdle() {
    MLE_D("waitDeviceIdle");
    std::scoped_lock lock(queue_data_[0].submit_mutex, queue_data_[1].submit_mutex, queue_data_[2].submit_mutex);
    check(Renderer::i().vkDevice().waitIdle());
}

vk::Result RendererCommandManager::submitPresent(const vk::PresentInfoKHR& present_info) {
    auto& q_data = queue_data_.at(queueDataIdx(GCmdType::G));
    std::scoped_lock lock(q_data.submit_mutex);
    return q_data.queue.presentKHR(present_info);
}

RendererCommandManager::QueueData::~QueueData() {
    std::scoped_lock lock(pool_map_mutex);

    for (auto& [_, pool_data] : thread_ots_pool_map) {
        if (pool_data.pool) {
            Renderer::i().destroy(pool_data.pool);
        }
    }
}
}  // namespace mle
