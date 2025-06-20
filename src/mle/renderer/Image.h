#pragma once

#include "Types.h"
#include "mle/common/Assert.h"
#include "mle/common/math/Types2D.h"
#include "mle/renderer/Buffer.h"
#include "mle/renderer/Renderer.h"

namespace mle::renderer {
/**
 * @brief Represents a Vulkan image with associated memory allocation.
 *
 * This class wraps a `vk::Image` and manages its lifetime, memory allocation and data uploads using VMA.
 * It supports different image usage types such as color attachment, depth attachment, and transfer operations.
 * There are a bunch of helper functions for updating the image data, transitioning its state, and reading image files.
 *
 * All images must be owned by the graphics queue! I will not perform any checks for that.
 * So just be mindful of that.
 * If you want to do comput work on an image, use the g queue. Ask for a GRAPHICS cmd pool.
 *
 * It is fine to bind images on mt, but state transitions are not thread-safe. I still dont know how to propper handle that.
 * For now just be really careful with that.
 */
class Image final : LiveCounter<Image> {
  public:
    /// Specifies parameters for image creation.
    struct CreateInfo {
        vk::Image o;                                      ///< Existing Vulkan image handle.
        vec2i extent;                                     ///< Image size in pixels.
        vk::Format format;                                ///< Pixel format.
        vk::ImageUsageFlags usage;                        ///< Vulkan usage flags.
        vk::MemoryPropertyFlags required_mem_flags = {};  ///< Required memory flags (optional).
    };
    using CI = CreateInfo;  ///< Alias for CreateInfo.

    /// Specifies parameters for creating an image view.
    struct ViewCreateInfo {};
    using ViewCI = ViewCreateInfo;  ///< Alias for ViewCreateInfo.

    /// Describes the current layout or usage of the image.
    enum class State : u8 {
        INITIAL,       ///< Undefined initial state.
        TRANSFER_SRC,  ///< Used as source in a transfer.
        TRANSFER_DST,  ///< Used as destination in a transfer.
        COLOR_ATT,     ///< Used as a color attachment.
        DEPTH_ATT,     ///< Used as a depth attachment.
        PRESENT,       ///< Presentable (swapchain).
        SHADER_READ    ///< Readable in shaders.
    };

  public:
    Image(const Image&) = default;
    Image& operator=(const Image&) = delete;
    Image(Image&&) = delete;
    Image& operator=(Image&&) = delete;

    /**
     * @brief Creates a Vulkan image with the specified configuration and returns a handle.
     * @param ci Image creation info.
     * @return An Image object initialized with the given parameters.
     */
    static ImageHnd createHnd(const CI& ci);

    /**
     * @brief Creates an empty image handle without initializing it.
     * Use init(ci) to set up the image parameters.
     */
    Image() = default;

    /// Destroys the image and associated resources.
    ~Image();

    /**
     * @brief Initializes the image with the given parameters.
     * @param ci Image creation info.
     */
    void init(const CI& ci);

    /**
     * @brief Uploads image data from a GPU buffer.
     *
     * Copies a region of data from a Vulkan buffer into this image. Both extent and offset are
     * in pixels. If `extent` is zero, the full image size is assumed.
     *
     * @param cmd Command buffer used for the copy.
     * @param buffer GPU buffer containing the source image data.
     * @param extent Size of the region to copy. Defaults to full image.
     * @param offset Offset in the destination image. Defaults to (0, 0).
     */
    void update(vk::CommandBuffer cmd, BufferRef buffer, vec2i extent = {0, 0}, vec2i offset = {0, 0});

    /// Same as `update(cmd, buffer, {extent.x, extent.y}, {offset.x, offset.y})` but using a rect.
    /// @see update(vk::CommandBuffer cmd, BufferRef buffer, vec2i extent, vec2i offset)
    void update(vk::CommandBuffer cmd, BufferRef buffer, Recti rect) { update(cmd, buffer, {rect.size.x, rect.size.y}, {rect.pos.x, rect.pos.y}); }

    void update(vk::CommandBuffer cmd, BufferRef buffer, Rectu rect) { update(cmd, buffer, {rect.size.x, rect.size.y}, {rect.pos.x, rect.pos.y}); }

    /**
     * @brief Performs a blit operation from another image.
     *
     * Copies a region of one image into this image.
     *
     * @param cmd Command buffer to record the blit.
     * @param src Source image.
     * @param src_rect Region of the source image to read from.
     * @param dst_rect Region of this image to write into.
     */
    void updateBlit(vk::CommandBuffer cmd, ImageRef src, Recti src_rect = {}, Recti dst_rect = {});

    /**
     * @brief Transitions the image to a new logical state.
     *
     * Performs a layout transition and memory barrier so the image can be used in the given state.
     *
     * @param cmd Command buffer to record the transition.
     * @param state New logical image state.
     */
    void transitionState(vk::CommandBuffer cmd, State state);

    /**
     * @brief Changes the ownership of the image to a different queue family.
     *
     * @param curr Current queue family index.
     * @param next Next queue family index to transfer ownership to.
     */
    void changeOwnerQueue(CmdType curr, vk::CommandBuffer curr_cmd, CmdType next, vk::CommandBuffer next_cmd);

    [[nodiscard]] auto getVkHnd() const { return o_; }        ///< Returns the Vulkan image handle.
    [[nodiscard]] auto get() const { return getVkHnd(); }     /// Alias for getVkHnd().
    [[nodiscard]] auto getExtent() const { return extent_; }  /// Returns the image size in pixels.
    /// Returns the image size in pixels as a Vulkan extent structure (3D).
    [[nodiscard]] vk::Extent3D getVkExtent3D() const { return {static_cast<u32>(extent_.x), static_cast<u32>(extent_.y), 1}; }
    /// Returns the image size in pixels as a Vulkan extent structure (2D).
    [[nodiscard]] vk::Extent2D getVkExtent2D() const { return {static_cast<u32>(extent_.x), static_cast<u32>(extent_.y)}; }
    [[nodiscard]] auto getFormat() const { return format_; }                 ///< Returns the image format
    [[nodiscard]] auto getImageUsage() const { return image_usage_; }        ///< Result the image usage flags.
    [[nodiscard]] auto getDefaultView() const { return default_view_; }      ///< Returns the default image view.
    [[nodiscard]] auto getCurrentLayout() const { return current_layout_; }  ///< Returns the current image layout.
    [[nodiscard]] u64 getAllocationSize() const;                             ///< Returns the size of the image allocation in bytes.
    [[nodiscard]] u64 getSizeInBytes() const;                                ///< Returns the size of the image in bytes (extent * format size).

    static int getFormatChannelCount(vk::Format format);
    static vk::Format getDefaultFormatForChannelCount(int c);

    struct FileInfo {
        vec2i extent;
        int channels;
    };
    [[nodiscard]] static FileInfo readFileInfo(const std::string& path);

    struct RawData {
        std::vector<u8> pixels;
        vec2i extent;
        int channels;
    };
    [[nodiscard]] static RawData readFile(const std::string& path, int target_channel_count = 0);

    [[nodiscard]] static BufferHnd createStagingBuffer(const void* data, vec2i extent, int channels);
    [[nodiscard]] static BufferHnd createStagingBuffer(const RawData& data) {
        return createStagingBuffer(rAs<const void*>(data.pixels.data()), data.extent, data.channels);
    }

  private:
    void initImage(const CI& ci);
    void initDefaultImageView();

    // This should not be private, but for now it is incomplete and should not be used outside
    [[nodiscard]] vk::ImageView createImageView() const;

    struct TransitionLayoutInfo {
        vk::ImageLayout new_layout;
        vk::PipelineStageFlags2 src_stage_mask;
        vk::AccessFlags2 src_access_mask;
        vk::PipelineStageFlags2 dst_stage_mask;
        vk::AccessFlags2 dst_access_mask;
    };
    void transitionLayout(vk::CommandBuffer cmd, TransitionLayoutInfo info);

  private:
    vk::Image o_;
    vec2i extent_{};
    vk::Format format_ = {};
    vk::ImageUsageFlags image_usage_;
    bool swapchain_ = false;

    VmaAllocation allocation_ = {};
    VmaAllocationInfo allocation_info_ = {};

    vk::ImageView default_view_;

    State current_state_ = State::INITIAL;

    vk::ImageLayout current_layout_ = vk::ImageLayout::eUndefined;
};

}  // namespace mle::renderer

namespace fmt {
using namespace mle::renderer;  // NOLINT
template <>
struct formatter<Image::State> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(Image::State v, FormatContext& ctx) const {
        switch (v) {
            case Image::State::INITIAL:
                return format_to(ctx.out(), "INITIAL");
            case Image::State::TRANSFER_SRC:
                return format_to(ctx.out(), "TRANSFER_SRC");
            case Image::State::TRANSFER_DST:
                return format_to(ctx.out(), "TRANSFER_DST");
            case Image::State::COLOR_ATT:
                return format_to(ctx.out(), "COLOR_ATT");
            case Image::State::PRESENT:
                return format_to(ctx.out(), "PRESENT");
            case Image::State::DEPTH_ATT:
                return format_to(ctx.out(), "DEPTH_ATT");
            case Image::State::SHADER_READ:
                return format_to(ctx.out(), "SHADER_READ");
        }
        MLE_TODO;
    }
};
}  // namespace fmt
