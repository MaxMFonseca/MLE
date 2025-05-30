#pragma once

#include "mle/common/math/Types2D.h"
#include "mle/renderer/Buffer.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Types.h"

namespace mle::renderer {
class Image final {
  public:
    struct CreateInfo {
        vec2i extent;
        vk::Format format;
        vk::ImageUsageFlags usage;
        vk::MemoryPropertyFlags required_mem_flags = {};
    };
    using CI = CreateInfo;

    struct CreateInfoSwapchain {
        vk::Image image;
    };
    using CISwapchain = CreateInfoSwapchain;

    struct ViewCreateInfo {};
    using ViewCI = ViewCreateInfo;

    enum class State : u8 { INITIAL, TRANSFER_SRC, TRANSFER_DST, COLOR_ATT, DEPTH_ATT, PRESENT, SHADER_READ };

  public:
    Image(const Image&) = default;
    Image(Image&&) = delete;
    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&) = delete;

    explicit Image(const CI& ci);
    explicit Image(const CISwapchain& ci);
    ~Image();

    void update(vk::CommandBuffer cmd, BufferRef buffer, vec2i extent = {0, 0}, vec2i offset = {0, 0});
    void update(vk::CommandBuffer cmd, BufferRef buffer, Recti rect) { update(cmd, buffer, {rect.size.x, rect.size.y}, {rect.pos.x, rect.pos.y}); }
    [[nodiscard]] BufferHnd update(vk::CommandBuffer cmd, const void* data, vec2i extent = {0, 0}, vec2i offset = {0, 0});
    auto update(vk::CommandBuffer cmd, const void* data, Recti rect) { return update(cmd, data, {rect.size.x, rect.size.y}, {rect.pos.x, rect.pos.y}); }
    void updateBlit(vk::CommandBuffer cmd, ImageRef src, Recti src_rect = {}, Recti dst_rect = {});

    void transitionState(vk::CommandBuffer cmd, State state);
    void transitionStateInFrame(State state) { transitionState(renderer::getFrameMainCommandBuffer(), state); }

    [[nodiscard]] auto getVkHnd() const { return obj_; }
    [[nodiscard]] auto get() const { return getVkHnd(); }
    [[nodiscard]] auto getExtent() const { return extent_; }
    [[nodiscard]] vk::Extent2D getVkExtent2D() const { return {static_cast<u32>(extent_.x), static_cast<u32>(extent_.y)}; }
    [[nodiscard]] auto getFormat() const { return format_; }
    [[nodiscard]] auto getImageUsage() const { return image_usage_; }
    [[nodiscard]] auto getDefaultView() const { return default_view_; }
    [[nodiscard]] auto getCurrentLayout() const { return current_layout_; }
    [[nodiscard]] u64 getAllocationSize() const;
    [[nodiscard]] u64 getSizeInBytes() const;

    static int getFormatChannelCount(vk::Format format);
    static vk::Format getDefaultFormatForChannelCount(int c);

    struct FileInfo {
        vec2i extent;
        int channels;
    };
    [[nodiscard]] static FileInfo readFileInfo(const fs::path& filepath);

    struct RawData {
        std::vector<u8> pixels;
        vec2i extent;
        int channels;
    };
    [[nodiscard]] static RawData readFile(const fs::path& filepath, int target_channel_count = 0);

  private:
    void createImage(const CI& ci);
    void createDefaultImageView();
    // void destroyImage();

    // This should not be private, but for now it is incomplete and should not be used outside
    [[nodiscard]] vk::ImageView createImageView() const noexcept;

    struct TransitionLayoutInfo {
        vk::ImageLayout new_layout;
        vk::PipelineStageFlags2 src_stage_mask;
        vk::AccessFlags2 src_access_mask;
        vk::PipelineStageFlags2 dst_stage_mask;
        vk::AccessFlags2 dst_access_mask;
    };
    void transitionLayout(vk::CommandBuffer cmd, TransitionLayoutInfo info);

  private:
    vk::Image obj_;
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

std::string toString(const Image::State& state);

}  // namespace mle::renderer

namespace fmt {
using namespace mle::renderer;

template <>
struct formatter<Image::State> {
    template <typename ParseContext>
    static constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(const Image::State& r, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", toString(r));
    }
};
}  // namespace fmt
