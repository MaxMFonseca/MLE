#include "Buffer.h"

#include <expected>

#include "mle/common/Assert.h"
#include "mle/common/Utils.h"
#include "mle/renderer/Renderer.h"

namespace mle::renderer {
Expected<BufferHnd> Buffer::create(const CI& ci) {
    MLE_T("Creating a buffer. size: {}, usage: {}", ci.size, vk::to_string(ci.usage));

    BufferHnd ret;

    vk::BufferCreateInfo buffer_ci{};
    buffer_ci.size = ci.size;
    buffer_ci.usage = ci.usage;

    VmaAllocationCreateInfo allocation_ci{};

    switch (ci.allocation_type) {
        case CI::AllocationType::GPU_ONLY:
            allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            ret->can_be_mapped_ = false;
            break;
        case CI::AllocationType::GPU_ONLY_HOST_WRITE_SEQ:
            allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            allocation_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            // TODO: check VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
            ret->can_be_mapped_ = true;
            break;
        case CI::AllocationType::GPU_ONLY_HOST_READ:
            allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            allocation_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            ret->can_be_mapped_ = true;
            break;
        case CI::AllocationType::STAGING:
            allocation_ci.usage = VMA_MEMORY_USAGE_AUTO;
            allocation_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            ret->can_be_mapped_ = true;
            break;
    }

    if (ci.dedicated_memory) {
        allocation_ci.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocation_ci.priority = 1.0F;
    }

    VkBuffer temp_buffer = VK_NULL_HANDLE;
    auto create_result = vmaCreateBuffer(getVma(), (VkBufferCreateInfo*)&buffer_ci, &allocation_ci,  // NOLINT
                                         &temp_buffer, &ret->allocation_, &ret->allocation_info_);
    if (create_result != VK_SUCCESS) {
        MLE_E("Failed to create a buffer. VkResult: {}", vk::Result(create_result));
        return std::unexpected(Result::VK_ERROR);
    }

    ret->obj_ = temp_buffer;
    ret->size_ = ci.size;
    ret->usage_ = ci.usage;
    ret->mapped_data_ = ret->allocation_info_.pMappedData;
    ret->persistent_ = ret->mapped_data_ != nullptr;

    MLE_T("Buffer created. VkHnd:{}, size: {}, usage: {}", (void*)ret->obj_, ret->size_, vk::to_string(ret->usage_));

    return ret;
}

Buffer::~Buffer() {
    if (!obj_) {
        return;
    }

    unmap();
    vmaDestroyBuffer(getVma(), obj_, allocation_);
}

Expected<void*> Buffer::map() {
    if (!mapped_data_) {
        auto map_result = vmaMapMemory(getVma(), allocation_, &mapped_data_);
        if (map_result != VK_SUCCESS) {
            MLE_E("Failed to map buffer memory. VkResult: {}", vk::Result(map_result));
            return std::unexpected(Result::VK_ERROR);
        }
    }
    return mapped_data_;
}

void Buffer::unmap() {
    if (mapped_data_ && !persistent_) {
        vmaUnmapMemory(getVma(), allocation_);
        mapped_data_ = nullptr;
    }
}

Result Buffer::update(const void* data, u64 data_size, u64 offset) {
    MLE_ASSERT(can_be_mapped_);

    if (data_size == max<u64>()) {
        data_size = size_ - offset;
    }
    MLE_ASSERT_LOG(data_size + offset <= size_, "Invalid buffer update. offset_({}) + size_({}) > m_size({})", offset, data_size, size_);

    auto map_r = map();
    if (!map_r) {
        MLE_E("Failed to map buffer memory for update. Error: {}", map_r.error());
        return map_r.error();
    }
    std::memcpy(getMappedWithOffset(offset), data, data_size);
    unmap();

    return Result::OK;
}

void Buffer::updateFromBuffer(vk::CommandBuffer cmd, BufferRef src, u64 data_size, u64 offset) {
    if (data_size == max<u64>()) {
        data_size = src->size_ - offset;
    }
    MLE_ASSERT_LOG(data_size + offset <= size_, "Invalid buffer update. offset_({}) + size_({}) > m_size({})", offset, data_size, size_);

    vk::BufferCopy copy_region{};
    copy_region.size = data_size;
    cmd.copyBuffer(src->get(), obj_, copy_region);
}

Expected<BufferHnd> Buffer::updateStaged(vk::CommandBuffer cmd, const void* data, u64 data_size, u64 offset) {
    if (data_size == max<u64>()) {
        data_size = size_ - offset;
    }
    MLE_ASSERT_LOG(data_size + offset <= size_, "Invalid buffer update. offset_({}) + size_({}) > m_size({})", offset, data_size, size_);

    CI staging_ci{};
    staging_ci.size = data_size;
    staging_ci.usage = vk::BufferUsageFlagBits::eTransferSrc;
    staging_ci.allocation_type = CI::AllocationType::STAGING;

    auto staging_buffer_r = create(staging_ci);
    if (!staging_buffer_r) {
        MLE_E("Failed to create staging buffer for update. Error: {}", staging_buffer_r.error());
        return std::unexpected(staging_buffer_r.error());
    }
    auto staging_buffer = std::move(staging_buffer_r.value());
    if (isError(staging_buffer->update(data, data_size))) {
        MLE_E("Failed to update staging buffer with data. Error: {}", staging_buffer_r.error());
        return std::unexpected(Result::VK_ERROR);
    }

    updateFromBuffer(cmd, staging_buffer.get(), data_size, offset);

    return staging_buffer;
}

vk::DescriptorBufferInfo Buffer::makeDescriptorInfo(u64 size, u64 offset) const {
    if (size == max<u64>()) {
        size = size_ - offset;
    }
    MLE_ASSERT_LOG(size + offset <= size_, "Invalid buffer update. offset_({}) + size_({}) > m_size({})", offset, size, size_);

    vk::DescriptorBufferInfo buffer_info{};
    buffer_info.buffer = obj_;
    buffer_info.offset = offset;
    buffer_info.range = size;
    return buffer_info;
}
}  // namespace mle::renderer
