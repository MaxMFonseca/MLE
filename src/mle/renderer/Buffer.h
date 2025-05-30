#pragma once

#include "Types.h"
#include "mle/renderer/VkContext.h"

namespace mle::renderer {
class Buffer final {
  public:
    struct CreateInfo {
        usize size;
        vk::BufferUsageFlags usage;

        bool dedicated_memory = false;

        enum class AllocationType : i8 { GPU_ONLY, GPU_ONLY_HOST_WRITE_SEQ, GPU_ONLY_HOST_READ, STAGING };
        AllocationType allocation_type;
    };
    using CI = CreateInfo;

  public:
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&) = delete;

    explicit Buffer(const CI& ci);
    ~Buffer();

    void* map();
    void unmap();

    void update(const void* data, u64 size = max<u64>(), u64 offset = 0);
    void updateFromBuffer(vk::CommandBuffer cmd, BufferRef src, u64 size = max<u64>(), u64 offset = 0);
    [[nodiscard]] BufferHnd updateStaged(vk::CommandBuffer cmd, const void* data, u64 size = max<u64>(), u64 offset = 0);

    [[nodiscard]] vk::DescriptorBufferInfo makeDescriptorInfo(u64 size = max<u64>(), u64 offset = 0) const;

    [[nodiscard]] const vk::Buffer& getVkHnd() { return obj_; }
    [[nodiscard]] const vk::Buffer& get() { return getVkHnd(); }
    [[nodiscard]] vk::BufferUsageFlags getUsage() const { return usage_; }
    [[nodiscard]] vk::DeviceSize getSize() const { return size_; }

  private:
    vk::Buffer obj_;

    vk::BufferUsageFlags usage_;
    VmaAllocation allocation_{};
    VmaAllocationInfo allocation_info_{};
    u64 size_ = 0;
    void* mapped_data_ = nullptr;
    bool persistent_ = false;
    bool can_be_mapped_ = false;
};
}  // namespace mle::renderer
