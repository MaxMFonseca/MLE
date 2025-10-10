#include "Buffer.h"

#include <vulkan/vulkan_structs.hpp>

#include "mle/core/Assert.h"
#include "mle/core/Logger.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Types.h"

namespace mle {
BufferHnd Buffer::createHnd(const CI& ci) {
    auto buffer = std::make_unique<Buffer>();
    buffer->create(ci);
    return buffer;
}

void Buffer::create(const CI& ci) {
    MLE_T("Creating a buffer. size: {}, usage: {}", ci.size, vk::to_string(ci.usage));

    vk::BufferCreateInfo buffer_ci{};
    buffer_ci.size = ci.size;
    buffer_ci.usage = ci.usage;
    buffer_ci.sharingMode = vk::SharingMode::eExclusive;

    VmaAllocationCreateInfo allocation_ci{};

    switch (ci.allocation_type) {
        case CI::AllocationType::GPU_ONLY:
            allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            can_be_mapped_ = false;
            break;
        case CI::AllocationType::GPU_ONLY_HOST_WRITE_SEQ:
            allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            allocation_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            can_be_mapped_ = true;
            break;
        case CI::AllocationType::GPU_ONLY_HOST_READ:
            allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            allocation_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            can_be_mapped_ = true;
            break;
        case CI::AllocationType::STAGING:
            allocation_ci.usage = VMA_MEMORY_USAGE_AUTO;
            allocation_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            can_be_mapped_ = true;
            break;
    }

    if (ci.dedicated_memory) {
        allocation_ci.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocation_ci.priority = 1.0F;
    }

    VkBuffer temp_buffer = VK_NULL_HANDLE;
    auto* vma = Renderer::i().vk().getVma();
    auto create_result = vmaCreateBuffer(vma, buffer_ci, &allocation_ci,  // NOLINT
                                         &temp_buffer, &allocation_, &allocation_info_);
    check(create_result);

    o_ = temp_buffer;
    size_ = ci.size;
    usage_ = ci.usage;
    mapped_data_ = allocation_info_.pMappedData;
    persistent_ = mapped_data_ != nullptr;

    MLE_T("Buffer created. VkHnd:{}, size: {}, usage: {}", (void*)o_, size_, vk::to_string(usage_));
}

Buffer::~Buffer() {
    if (!o_) {
        return;
    }
    unmap();
    vmaDestroyBuffer(Renderer::i().vk().getVma(), o_, allocation_);
}

void* Buffer::map() {
    if (!mapped_data_) {
        check(vmaMapMemory(Renderer::i().vk().getVma(), allocation_, &mapped_data_));
    }
    return mapped_data_;
}

void Buffer::unmap() {
    if (mapped_data_ && !persistent_) {
        vmaUnmapMemory(Renderer::i().vk().getVma(), allocation_);
        mapped_data_ = nullptr;
    }
}

void Buffer::write(const void* data, usize data_size, usize offset) {
    MLE_ASSERT(can_be_mapped_);

    if (data_size == max<u64>()) {
        data_size = size_ - offset;
    }
    MLE_ASSERT_LOG(data_size + offset <= size_, "Invalid buffer update. offset_({}) + size_({}) > m_size({})", offset, data_size, size_);

    map();
    std::memcpy(getMappedOffset(offset), data, data_size);
    unmap();
}

void Buffer::copy(CommandBuffer& cmd, BufferRef src, usize size, usize src_offset, usize dst_offset) {
    if (size == max<usize>()) {
        size = src->getSize() - src_offset;
    }

    MLE_ASSERT(queue_data_idx_ != INVALID_QUEUE && src->queue_data_idx_ != INVALID_QUEUE);
    MLE_ASSERT_LOG(dst_offset + size <= size_, "Invalid buffer copy. dst_offset({}) + size({}) > m_size({})", dst_offset, size, size_);
    MLE_ASSERT_LOG(src_offset + size <= src->getSize(), "Invalid buffer copy. src_offset({}) + size({}) > src.m_size({})", src_offset, size, src->getSize());
    MLE_ASSERT_LOG(queue_data_idx_ == NO_QUEUE || queue_data_idx_ == cmd.queueDataIdx(),
                   "Buffer copy across different queue families is not supported, transfer the ownerships. buffer: {}, cmd: {}", queue_data_idx_,
                   cmd.queueDataIdx());

    vk::BufferCopy copy_region{};
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    copy_region.size = size;

    cmd().copyBuffer(src->get(), o_, copy_region);

    queue_data_idx_ = cmd.queueDataIdx();
}

BufferHnd Buffer::writeStaged(CommandBuffer& cmd, const void* data, usize size, usize src_offset, usize dst_offset) {
    if (size == max<usize>()) {
        size = size_ - dst_offset;
    }

    MLE_ASSERT_LOG(dst_offset + size <= size_, "Invalid buffer staged write. dst_offset({}) + size({}) > m_size({})", dst_offset, size, size_);
    MLE_ASSERT_LOG(src_offset + size <= size, "Invalid buffer staged write. src_offset({}) + size({}) > size({})", src_offset, size, size);

    CI staging_ci;
    staging_ci.size = size;
    staging_ci.usage = vk::BufferUsageFlagBits::eTransferSrc;
    staging_ci.allocation_type = CI::AllocationType::STAGING;

    auto staging_buffer = Buffer::createHnd(staging_ci);
    staging_buffer->write(data, size, src_offset);

    copy(cmd, staging_buffer.get(), size, 0, dst_offset);

    return staging_buffer;
}

vk::DescriptorBufferInfo Buffer::makeDescriptorInfo(CommandBuffer& cmd, usize size, usize offset) {
    MLE_ASSERT(queue_data_idx_ != INVALID_QUEUE);

    if (size == max<usize>()) {
        size = size_ - offset;
    }
    MLE_ASSERT_LOG(size + offset <= size_, "Invalid buffer update. offset_({}) + size_({}) > m_size({})", offset, size, size_);

    if (queue_data_idx_ == NO_QUEUE) {
        queue_data_idx_ = cmd.queueDataIdx();
    }

    MLE_ASSERT_LOG(queue_data_idx_ == cmd.queueDataIdx(),
                   "Buffer used in descriptor across different queue families is not supported, transfer the ownerships. buffer: {}, cmd: {}", queue_data_idx_,
                   cmd.queueDataIdx());

    vk::DescriptorBufferInfo buffer_info{};
    buffer_info.buffer = o_;
    buffer_info.offset = offset;
    buffer_info.range = size;

    return buffer_info;
}

void* Buffer::getMappedOffset(usize offset) {
    MLE_ASSERT(offset < size_);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) kind of the point of this function
    return as<u8*>(map()) + offset;
}

vk::DeviceAddress Buffer::getDeviceAddress() {
    return Renderer::i().vk().getDevice().getBufferAddress({o_});
}

Semaphore Buffer::releaseOwnership(usize new_family) {
    usize src_queue_family = Renderer::i().cmdMgr().queueFamilyIdx(queue_data_idx_);

    vk::BufferMemoryBarrier2 mb{};
    mb.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    mb.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead;
    mb.dstStageMask = vk::PipelineStageFlagBits2::eNone;
    mb.dstAccessMask = vk::AccessFlagBits2::eNone;
    mb.srcQueueFamilyIndex = src_queue_family;
    mb.dstQueueFamilyIndex = new_family;
    mb.buffer = o_;
    mb.offset = 0;
    mb.size = size_;

    vk::DependencyInfo dep{};
    dep.bufferMemoryBarrierCount = 1;
    dep.pBufferMemoryBarriers = &mb;

    auto cmd = Renderer::i().cmdMgr().getOTS(queue_data_idx_);

    cmd().pipelineBarrier2(dep);

    vk::CommandBufferSubmitInfo command_buffer_info{};
    command_buffer_info.commandBuffer = cmd.get();
    command_buffer_info.deviceMask = 0;

    vk::SubmitInfo2 submit_info{};
    submit_info.setCommandBufferInfos(command_buffer_info);

    auto semaphore = Renderer::i().syncMgr().acquireSemaphore();

    vk::SemaphoreSubmitInfo signal_info{};
    signal_info.semaphore = semaphore.get();
    signal_info.stageMask = vk::PipelineStageFlagBits2::eAllCommands;
    submit_info.setSignalSemaphoreInfos(signal_info);

    Renderer::i().cmdMgr().submitOTSAsync(std::move(cmd), submit_info);

    queue_data_idx_ = INVALID_QUEUE;

    return semaphore;
}

std::optional<Semaphore> Buffer::acquireOwnership(CommandBuffer& cmd, vk::PipelineStageFlags2 dst_stage_mask, vk::AccessFlags2 dst_access_mask) {
    if (queue_data_idx_ == NO_QUEUE) {
        queue_data_idx_ = cmd.queueDataIdx();
        return {};
    }
    if (queue_data_idx_ == cmd.queueDataIdx()) {
        return {};
    }

    usize src_queue_family = Renderer::i().cmdMgr().queueFamilyIdx(queue_data_idx_);
    usize dst_queue_family = Renderer::i().cmdMgr().queueFamilyIdx(cmd.queueDataIdx());

    auto semaphore = releaseOwnership(dst_queue_family);

    vk::BufferMemoryBarrier2 mb;
    mb.srcStageMask = vk::PipelineStageFlagBits2::eNone;
    mb.srcAccessMask = vk::AccessFlagBits2::eNone;
    mb.dstStageMask = dst_stage_mask;
    mb.dstAccessMask = dst_access_mask;
    mb.srcQueueFamilyIndex = src_queue_family;
    mb.dstQueueFamilyIndex = dst_queue_family;
    mb.buffer = o_;
    mb.offset = 0;
    mb.size = size_;

    vk::DependencyInfo dep{};
    dep.bufferMemoryBarrierCount = 1;
    dep.pBufferMemoryBarriers = &mb;

    cmd().pipelineBarrier2(dep);

    queue_data_idx_ = cmd.queueDataIdx();

    return semaphore;
}

void Buffer::transferOwnershipOTSWait(GCmdType type) {
    MLE_ASSERT(queue_data_idx_ != INVALID_QUEUE);

    usize queue_data_idx = Renderer::i().cmdMgr().queueDataIdx(type);
    if (queue_data_idx_ == NO_QUEUE) {
        queue_data_idx_ = queue_data_idx;
        return;
    }
    if (queue_data_idx_ == queue_data_idx) {
        return;
    }

    auto cmd = Renderer::i().cmdMgr().getOTS(queue_data_idx);

    auto semaphore = acquireOwnership(cmd);

    vk::SemaphoreSubmitInfo wait_info{};
    if (semaphore.has_value()) {
        wait_info.semaphore = semaphore->get();
        wait_info.stageMask = vk::PipelineStageFlagBits2::eAllCommands;
    }

    vk::CommandBufferSubmitInfo command_buffer_info{};
    command_buffer_info.commandBuffer = cmd.get();
    command_buffer_info.deviceMask = 0;

    vk::SubmitInfo2 submit_info{};
    if (semaphore.has_value()) {
        submit_info.setWaitSemaphoreInfos(wait_info);
    }
    submit_info.setCommandBufferInfos(command_buffer_info);

    Renderer::i().cmdMgr().submitOTSWait(std::move(cmd), submit_info);
}

}  // namespace mle
