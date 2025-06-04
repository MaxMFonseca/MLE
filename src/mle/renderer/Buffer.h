/**
 * @file
 * @brief Vulkan buffer wrapper using VMA for memory allocation.
 */

#pragma once

#include "Types.h"
#include "mle/common/LiveCounter.h"
#include "mle/common/Result.h"
#include "mle/common/Utils.h"

namespace mle::renderer {
/**
 * @brief Represents a Vulkan buffer with associated memory allocation.
 *
 * This class wraps a `vk::Buffer` and manages its lifetime, memory mapping,
 * and data uploads using VMA. It supports different allocation types for staging,
 * GPU-only, and host-visible memory.
 */
class Buffer final : public LiveCounter<Buffer> {
  public:
    /// Configuration structure for buffer creation.
    struct CreateInfo {
        usize size;                  ///< Size of the buffer in bytes.
        vk::BufferUsageFlags usage;  ///< Vulkan buffer usage flags.

        bool dedicated_memory = false;  ///< Whether to use a dedicated memory allocation.

        /// Specifies the memory allocation strategy for the buffer.
        enum class AllocationType : i8 {
            GPU_ONLY,                 ///< Allocated only on the GPU, not accessible by host.
            GPU_ONLY_HOST_WRITE_SEQ,  ///< GPU memory with host-visible sequential write access.
            GPU_ONLY_HOST_READ,       ///< GPU memory with host-visible read access.
            STAGING                   ///< CPU-visible staging buffer for data upload.
        };

        AllocationType allocation_type;  ///< Memory allocation strategy.
    };
    using CI = CreateInfo;  ///< Alias for CreateInfo.

  public:
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&) = delete;

    Buffer(Buffer&& other);

    /**
     * @brief Creates a Vulkan buffer with the specified configuration.
     *
     * @param ci Buffer creation configuration.
     * @return a Buffer instance
     */
    static Buffer create(const CI& ci);

    /**
     * @brief Creates a Vulkan buffer with the specified configuration and returns a handle.
     *
     * @param ci Buffer creation configuration.
     * @return a BufferHnd containing the created buffer.
     */
    static BufferHnd createHnd(const CI& ci);

    /**
     * @brief Initializes the buffer with the provided creation info.
     *
     * @param ci Buffer creation configuration.
     */
    void init(const CI& ci);

    /**
     * @brief Releases the Vulkan buffer and its associated memory.
     */
    void release();

    /// Destroys the Vulkan buffer and frees its memory.
    ~Buffer();

    /**
     * @brief Maps the buffer memory and returns a pointer to the mapped region.
     *
     * @return an `Expected<void*>` containing the mapped memory pointer or an error.
     */
    void* map();

    /// Unmaps the buffer memory if it was previously mapped.
    void unmap();

    /**
     * @brief Updates the buffer with raw data.
     * @param data Pointer to the data.
     * @param size Size of the data to copy. Defaults to entire buffer.
     * @param offset Offset in the buffer to start writing to.
     */
    void update(const void* data, u64 size = max<u64>(), u64 offset = 0);

    /**
     * @brief Copies data from another buffer using a command buffer.
     * @param cmd The command buffer used for the copy.
     * @param src Source buffer to copy from.
     * @param size Number of bytes to copy. Defaults to entire buffer.
     * @param offset Offset in the destination buffer.
     */
    void update(vk::CommandBuffer cmd, BufferRef src, u64 size = max<u64>(), u64 offset = 0);

    /**
     * @brief Uploads data using a temporary staging buffer and a copy command.
     *
     * @param cmd Command buffer used to record the copy.
     * @param data Pointer to source data.
     * @param size Size of data to copy. Defaults to entire buffer.
     * @param offset Offset in the destination buffer.
     * @return A temporary staging buffer that must be kept alive until the copy completes or an error.
     */
    [[nodiscard]] Buffer updateStaged(vk::CommandBuffer cmd, const void* data, u64 size = max<u64>(), u64 offset = 0);

    /**
     * @brief Returns a descriptor buffer info used for descriptor bindings.
     * @param size Size of the buffer to expose. Defaults to full size.
     * @param offset Offset within the buffer. Defaults to 0.
     * @return Vulkan descriptor buffer info.
     */
    [[nodiscard]] vk::DescriptorBufferInfo makeDescriptorInfo(u64 size = max<u64>(), u64 offset = 0) const;

    [[nodiscard]] const vk::Buffer& getVkHnd() { return obj_; }                         ///< Returns the Vulkan buffer handle.
    [[nodiscard]] const vk::Buffer& get() { return getVkHnd(); }                        ///< Alias for getVkHnd().
    [[nodiscard]] vk::BufferUsageFlags getUsage() const { return usage_; }              ///< Returns the buffer usage flags.
    [[nodiscard]] vk::DeviceSize getSize() const { return size_; }                      ///< Returns the total size of the buffer in bytes.
    void* getMappedWithOffset(usize offset) { return as<u8*>(mapped_data_) + offset; }  ///< Returns a pointer to the mapped memory with an offset.

  private:
    /**
     * @brief Constructs a Vulkan buffer from the provided creation info.
     * @param ci Buffer creation configuration.
     */
    Buffer() = default;

  private:
    vk::Buffer obj_;  ///< Vulkan buffer handle.

    vk::BufferUsageFlags usage_;           ///< Usage flags for the Vulkan buffer.
    VmaAllocation allocation_{};           ///< VMA allocation handle.
    VmaAllocationInfo allocation_info_{};  ///< VMA allocation info for mapped memory access.
    u64 size_ = 0;                         ///< Size of the buffer in bytes.
    void* mapped_data_ = nullptr;          ///< Pointer to mapped memory if applicable.
    bool persistent_ = false;              ///< Whether the buffer stays mapped for its lifetime.
    bool can_be_mapped_ = false;           ///< Whether the buffer memory is host-visible.
};
}  // namespace mle::renderer
