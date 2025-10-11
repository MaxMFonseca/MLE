#pragma once

#include "Types.h"
#include "mle/renderer/SyncManager.h"
#include "mle/utils/Utils.h"

namespace mle {
class Buffer {
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
    MLE_NO_COPY(Buffer);

    Buffer() = default;
    ~Buffer();

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    void create(const CI& ci);

    void* map();
    void unmap();

    void write(const void* data, usize size, usize offset = 0);
    void copy(CommandBuffer& cmd, BufferRef src, usize size = max<usize>(), usize src_offset = 0, usize dst_offset = 0);
    [[nodiscard]] BufferHnd writeStaged(CommandBuffer& cmd, const void* data, usize size, usize src_offset = 0, usize dst_offset = 0);

    [[nodiscard]] vk::DescriptorBufferInfo makeDescriptorInfo(CommandBuffer& cmd, usize size = max<usize>(), usize offset = 0);

    [[nodiscard]] vk::Buffer get() { return o_; }
    [[nodiscard]] vk::BufferUsageFlags getUsage() const { return usage_; }
    [[nodiscard]] vk::DeviceSize getSize() const { return size_; }
    [[nodiscard]] bool isPersistent() const { return persistent_; }

    void* getMappedOffset(usize offset);

    vk::DeviceAddress getDeviceAddress();

    void ownershipRelease(CommandBuffer& cmd, QueueDataIdx dst_queue_data_idx);
    [[nodiscard]] std::optional<Semaphore> ownershipReleaseOTS(QueueDataIdx dst_queue_data_idx);
    void ownershipAcquireOTSWait(QueueDataIdx dst_queue_data_idx);
    void ownershipAcquire(CommandBuffer& cmd, vk::PipelineStageFlags2 dst_stage_mask = vk::PipelineStageFlagBits2::eAllCommands,
                          vk::AccessFlags2 dst_access_mask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead);
    [[nodiscard]] std::optional<Semaphore> ownershipReleaseOTSAcquire(CommandBuffer& cmd,
                                                                      vk::PipelineStageFlags2 dst_stage_mask = vk::PipelineStageFlagBits2::eAllCommands,
                                                                      vk::AccessFlags2 dst_access_mask = vk::AccessFlagBits2::eMemoryWrite |
                                                                                                         vk::AccessFlagBits2::eMemoryRead);
    void ownershipReleaseOTSAcquireOTSWait(GCmdType type);

    [[nodiscard]] static BufferHnd createHnd(const CI& ci);

  private:
    vk::Buffer o_;

    vk::BufferUsageFlags usage_;
    VmaAllocation allocation_{};
    VmaAllocationInfo allocation_info_{};
    usize size_ = 0;
    QueueDataIdx queue_data_idx_ = NO_QUEUE;
    QueueDataIdx prev_queue_data_idx_ = NO_QUEUE;
    void* mapped_data_ = nullptr;
    bool persistent_ = false;
    bool can_be_mapped_ = false;
};
}  // namespace mle
