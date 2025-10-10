#include "Buffer.h"

#include <vulkan/vulkan_core.h>

#include <expected>
#include <vulkan/vulkan.hpp>

#include "mle/common/Assert.h"
#include "mle/common/Utils.h"
#include "mle/core/Core.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Utils.h"

namespace mle::renderer {
BufferHnd Buffer::createHnd(const CI& ci) {
    auto ret = std::make_unique<Buffer>();
    ret->init(ci);
    return ret;
}

void Buffer::init(const CI& ci) {
    MLE_T("Creating a buffer. size: {}, usage: {}", ci.size, vk::to_string(ci.usage));

    vk::BufferCreateInfo buffer_ci{};
    buffer_ci.size = ci.size;
    buffer_ci.usage = ci.usage;

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
    auto create_result = vmaCreateBuffer(detail::getVma(), (VkBufferCreateInfo*)&buffer_ci, &allocation_ci,  // NOLINT
                                         &temp_buffer, &allocation_, &allocation_info_);
    if (create_result != VK_SUCCESS) {
        core::unrecoverable("Failed to create buffer. VkResult: {}", vk::Result(create_result));
    }

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
    vmaDestroyBuffer(detail::getVma(), o_, allocation_);
}

void* Buffer::map() {
    if (!mapped_data_) {
        auto map_result = vmaMapMemory(detail::getVma(), allocation_, &mapped_data_);
        if (map_result != VK_SUCCESS) {
            core::unrecoverable("Failed to map buffer memory. VkResult: {}", vk::Result(map_result));
        }
    }
    return mapped_data_;
}

void Buffer::unmap() {
    if (mapped_data_ && !persistent_) {
        vmaUnmapMemory(detail::getVma(), allocation_);
        mapped_data_ = nullptr;
    }
}

void Buffer::update(const void* data, u64 data_size, u64 offset) {
    MLE_ASSERT(can_be_mapped_);

    if (data_size == max<u64>()) {
        data_size = size_ - offset;
    }
    MLE_ASSERT_LOG(data_size + offset <= size_, "Invalid buffer update. offset_({}) + size_({}) > m_size({})", offset, data_size, size_);

    map();
    std::memcpy(getMappedWithOffset(offset), data, data_size);
    unmap();
}

void Buffer::update(vk::CommandBuffer cmd, BufferRef src, u64 data_size, u64 offset) {
    if (data_size == max<u64>()) {
        data_size = src->size_ - offset;
    }
    MLE_ASSERT_LOG(data_size + offset <= size_, "Invalid buffer update. offset_({}) + size_({}) > m_size({})", offset, data_size, size_);

    vk::BufferCopy copy_region{};
    copy_region.size = data_size;
    cmd.copyBuffer(src->get(), o_, copy_region);
}

BufferHnd Buffer::updateStaged(vk::CommandBuffer cmd, const void* data, u64 data_size, u64 offset) {
    if (data_size == max<u64>()) {
        data_size = size_ - offset;
    }
    MLE_ASSERT_LOG(data_size + offset <= size_, "Invalid buffer update. offset_({}) + size_({}) > m_size({})", offset, data_size, size_);

    CI staging_ci{};
    staging_ci.size = data_size;
    staging_ci.usage = vk::BufferUsageFlagBits::eTransferSrc;
    staging_ci.allocation_type = CI::AllocationType::STAGING;

    auto staging_buffer = createHnd(staging_ci);
    staging_buffer->update(data, data_size, 0);

    update(cmd, staging_buffer.get(), data_size, offset);

    return staging_buffer;
}

vk::DescriptorBufferInfo Buffer::makeDescriptorInfo(u64 size, u64 offset) const {
    if (size == max<u64>()) {
        size = size_ - offset;
    }
    MLE_ASSERT_LOG(size + offset <= size_, "Invalid buffer update. offset_({}) + size_({}) > m_size({})", offset, size, size_);

    vk::DescriptorBufferInfo buffer_info{};
    buffer_info.buffer = o_;
    buffer_info.offset = offset;
    buffer_info.range = size;
    return buffer_info;
}

vk::DeviceAddress Buffer::getDeviceAddress() {
    return detail::getDevice().getBufferAddress({o_});
}
}  // namespace mle::renderer
