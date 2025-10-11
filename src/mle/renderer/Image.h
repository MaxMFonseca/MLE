#pragma once

#include "Types.h"
#include "mle/core/Assert.h"
#include "mle/math/Types2D.h"
#include "mle/renderer/SyncManager.h"
#include "mle/renderer/Utils.h"
#include "mle/utils/Utils.h"

namespace mle {
struct ImageViewCreateInfo {
    vk::ComponentSwizzle r = vk::ComponentSwizzle::eIdentity;
    vk::ComponentSwizzle g = vk::ComponentSwizzle::eIdentity;
    vk::ComponentSwizzle b = vk::ComponentSwizzle::eIdentity;
    vk::ComponentSwizzle a = vk::ComponentSwizzle::eIdentity;
};

class Image final {
  public:
    enum class State : u8 {
        INITIAL,       ///< Undefined initial state.
        PRESENT_SRC,   ///< Used as source in a present operation.
        TRANSFER_SRC,  ///< Used as source in a transfer.
        TRANSFER_DST,  ///< Used as destination in a transfer.
        COLOR_ATT,     ///< Used as a color attachment.
        DEPTH_ATT,     ///< Used as a depth attachment.
        COMPUTE_RW,    ///< Compute read/write.
        COMPUTE_R,     ///< Compute read only
        FS_READ        ///< Readable in shaders.
    };

    struct StateProps {
        vk::ImageLayout layout;
        vk::PipelineStageFlags2 stage;
        vk::AccessFlags2 access;
    };

    using Format = ImageFormat;

    struct RawData {
        vec2u extent;
        int channels;
        Bytes pixels;
    };

    struct CreateInfo {
        vk::Image o;
        vec2u extent;
        ImageFormat format;
        vk::ImageUsageFlags extra_usage;
    };
    using CI = CreateInfo;

    using ViewCI = ImageViewCreateInfo;

  public:
    Image() = default;
    ~Image();

    Image(Image&& other);
    Image& operator=(Image&& other);

    MLE_NO_COPY(Image);

    void init(const CI& ci);

    void copyBuffer(CommandBuffer& cmd, Buffer& src, vec2u extent = {0, 0}, vec2i offset = {0, 0});
    void copyBuffer(CommandBuffer& cmd, Buffer& src, Recti rect) { copyBuffer(cmd, src, vec2u{rect.size.x, rect.size.y}, {rect.pos.x, rect.pos.y}); }
    void copyImage(CommandBuffer& cmd, Image& src, vec2u extent = {0, 0}, vec2i src_offset = {0, 0}, vec2i dst_offset = {0, 0});
    void blitImage(CommandBuffer& cmd, Image& src, Recti src_rect, Recti dst_rect);
    BufferHnd copyToBufferOTS(vec2u extent = {0, 0}, vec2i offset = {0, 0});

    [[nodiscard]] BufferHnd copyRaw(CommandBuffer& cmd, const RawData& data, vec2i offset = {0, 0});

    void clear(CommandBuffer& cmd, vk::ClearColorValue color);
    void clear(CommandBuffer& cmd, const Color& color) { clear(cmd, toVkColor(color)); }
    void clear(CommandBuffer& cmd, vk::ClearDepthStencilValue depth);

    void transitionState(CommandBuffer& cmd, State state);

    void ownershipRelease(CommandBuffer& cmd, usize dst_queue_data_idx);
    [[nodiscard]] std::optional<Semaphore> ownershipReleaseOTS(usize dst_queue_data_idx);
    void ownershipAcquireOTSWait(usize dst_queue_data_idx);
    void ownershipAcquire(CommandBuffer& cmd, vk::PipelineStageFlags2 dst_stage_mask = vk::PipelineStageFlagBits2::eAllCommands,
                          vk::AccessFlags2 dst_access_mask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead);
    [[nodiscard]] std::optional<Semaphore> ownershipReleaseOTSAcquire(CommandBuffer& cmd,
                                                                      vk::PipelineStageFlags2 dst_stage_mask = vk::PipelineStageFlagBits2::eAllCommands,
                                                                      vk::AccessFlags2 dst_access_mask = vk::AccessFlagBits2::eMemoryWrite |
                                                                                                         vk::AccessFlagBits2::eMemoryRead);
    void ownershipReleaseOTSAcquireOTSWait(GCmdType type);

    [[nodiscard]] vk::Image get() const { return o_; }
    [[nodiscard]] vk::Format getFormat() const { return vk_format_; }
    [[nodiscard]] vk::ImageUsageFlags getUsage() const { return usage_; }
    [[nodiscard]] vec2u getExtent() const { return extent_; }
    [[nodiscard]] usize getQueueDataIdx() const { return queue_data_idx_; }
    [[nodiscard]] State getCurrentState() const { return state_; }
    [[nodiscard]] vk::ImageLayout getLayout() const { return layout_; }
    [[nodiscard]] vk::Extent2D getVkExtent() const { return {extent_.x, extent_.y}; }
    [[nodiscard]] vk::Extent3D getVkExtent3D() const { return {extent_.x, extent_.y, 1}; }
    [[nodiscard]] f32 aspect() const { return static_cast<f32>(extent_.x) / static_cast<f32>(extent_.y); }

    [[nodiscard]] VmaAllocation getAllocation() const { return allocation_; }
    [[nodiscard]] VmaAllocationInfo getAllocationInfo() const { return allocation_info_; }

    [[nodiscard]] vk::ImageView getDefaultView() const { return views_.at(0); }
    [[nodiscard]] vk::ImageView createView(const ViewCI& ci = {});

    [[nodiscard]] int getChannelCount() const;

    static RawData readFile(const std::string& path, int desired_channels = 0);

    static ImageHnd createHnd(const CI& ci);

  private:
    struct TransitionLayoutInfo {
        vk::ImageLayout new_layout;
        vk::PipelineStageFlags2 src_stage_mask;
        vk::AccessFlags2 src_access_mask;
        vk::PipelineStageFlags2 dst_stage_mask;
        vk::AccessFlags2 dst_access_mask;
    };
    void transitionLayout(CommandBuffer& cmd, TransitionLayoutInfo info);
    void checkQueueOwnership(CommandBuffer& cmd);

    static constexpr StateProps getStateProps(State state);

  private:
    vk::Image o_{};
    vk::Format vk_format_{};
    ImageFormat format_ = ImageFormat::COUNT;
    vk::ImageUsageFlags usage_;
    usize queue_data_idx_ = NO_QUEUE;
    usize prev_queue_data_idx_ = NO_QUEUE;
    vec2u extent_{};
    VmaAllocation allocation_ = {};
    VmaAllocationInfo allocation_info_ = {};
    State state_ = State::INITIAL;
    vk::ImageLayout layout_ = vk::ImageLayout::eUndefined;

    std::vector<vk::ImageView> views_;
};
}  // namespace mle

namespace fmt {
template <>
struct formatter<mle::Image::State> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::Image::State v, FormatContext& ctx) const {
        using mle::Image;
        switch (v) {
            case Image::State::INITIAL:
                return format_to(ctx.out(), "INITIAL");
            case Image::State::PRESENT_SRC:
                return format_to(ctx.out(), "PRESENT_SRC");
            case Image::State::TRANSFER_SRC:
                return format_to(ctx.out(), "TRANSFER_SRC");
            case Image::State::TRANSFER_DST:
                return format_to(ctx.out(), "TRANSFER_DST");
            case Image::State::COLOR_ATT:
                return format_to(ctx.out(), "COLOR_ATT");
            case Image::State::DEPTH_ATT:
                return format_to(ctx.out(), "DEPTH_ATT");
            case Image::State::COMPUTE_RW:
                return format_to(ctx.out(), "COMPUTE_RW");
            case Image::State::COMPUTE_R:
                return format_to(ctx.out(), "COMPUTE_R");
            case Image::State::FS_READ:
                return format_to(ctx.out(), "FS_READ");
        }
        MLE_TODO;
    }
};
}  // namespace fmt
