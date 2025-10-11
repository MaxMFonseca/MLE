#include "Image.h"

#include <stb_image.h>

#include <cstring>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "Buffer.h"
#include "Utils.h"
#include "VkCtx.h"
#include "mle/renderer/Renderer.h"

namespace mle {
ImageHnd Image::createHnd(const CI& ci) {
    auto img = std::make_unique<Image>();
    img->init(ci);
    return img;
}

Image::~Image() {
    if (!o_) {
        return;
    }
    for (auto v : views_) {
        Renderer::i().destroy(v);
    }
    if (allocation_) {
        vmaDestroyImage(Renderer::i().vk().getVma(), o_, allocation_);
    }
}

Image::Image(Image&& other) noexcept :
    o_(other.o_),
    vk_format_(other.vk_format_),
    format_(other.format_),
    usage_(other.usage_),
    queue_data_idx_(other.queue_data_idx_),
    extent_(other.extent_),
    allocation_(other.allocation_),
    allocation_info_(other.allocation_info_),
    state_(other.state_),
    views_(std::move(other.views_)) {
    other.o_ = nullptr;
    other.allocation_ = {};
    other.allocation_info_ = {};
    other.views_.clear();
}

Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) {
        o_ = other.o_;
        vk_format_ = other.vk_format_;
        format_ = other.format_;
        usage_ = other.usage_;
        queue_data_idx_ = other.queue_data_idx_;
        extent_ = other.extent_;
        allocation_ = other.allocation_;
        allocation_info_ = other.allocation_info_;
        state_ = other.state_;
        views_ = std::move(other.views_);
        other.o_ = nullptr;
        other.allocation_ = {};
        other.allocation_info_ = {};
        other.views_.clear();
    }
    return *this;
}

void Image::init(const CI& ci) {
    extent_ = ci.extent;
    format_ = ci.format;
    vk_format_ = Renderer::i().vk().getVkImageFormat(ci.format);
    usage_ = Renderer::i().vk().getVkImageUsage(ci.format) | ci.extra_usage;

    if (!ci.o) {
        vk::ImageCreateInfo image_ci{};
        image_ci.imageType = vk::ImageType::e2D;
        image_ci.format = vk_format_;
        image_ci.extent.width = extent_.x;
        image_ci.extent.height = extent_.y;
        image_ci.extent.depth = 1;
        image_ci.mipLevels = 1;
        image_ci.arrayLayers = 1;
        image_ci.samples = vk::SampleCountFlagBits::e1;
        image_ci.tiling = vk::ImageTiling::eOptimal;
        image_ci.usage = usage_;
        image_ci.initialLayout = vk::ImageLayout::eUndefined;
        image_ci.sharingMode = vk::SharingMode::eExclusive;

        VmaAllocationCreateInfo alloc_ci{};
        alloc_ci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        alloc_ci.priority = 1.0F;

        VkImage vk_image = VK_NULL_HANDLE;

        auto create_result = vmaCreateImage(Renderer::i().vk().getVma(), image_ci, &alloc_ci, &vk_image, &allocation_, &allocation_info_);
        check(create_result);
        o_ = vk_image;

    } else {
        o_ = ci.o;
    }
    auto _ = createView();
}

vk::ImageView Image::createView(const ViewCI& ci) {
    vk::ImageViewCreateInfo image_view_ci{};
    image_view_ci.image = o_;
    image_view_ci.viewType = vk::ImageViewType::e2D;
    image_view_ci.format = vk_format_;
    image_view_ci.components.r = ci.r;
    image_view_ci.components.g = ci.g;
    image_view_ci.components.b = ci.b;
    image_view_ci.components.a = ci.a;
    image_view_ci.subresourceRange.aspectMask =
        (usage_ & vk::ImageUsageFlagBits::eDepthStencilAttachment) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    image_view_ci.subresourceRange.baseMipLevel = 0;
    image_view_ci.subresourceRange.levelCount = 1;
    image_view_ci.subresourceRange.baseArrayLayer = 0;
    image_view_ci.subresourceRange.layerCount = 1;

    vk::ImageView vkhnd = unwrap(Renderer::i().vk().getDevice().createImageView(image_view_ci));
    views_.push_back(vkhnd);
    return vkhnd;
}

void Image::checkQueueOwnership(CommandBuffer& cmd) {
    if (queue_data_idx_ == NO_QUEUE) {
        queue_data_idx_ = cmd.queueDataIdx();
    }
    MLE_ASSERT_LOG(queue_data_idx_ == cmd.queueDataIdx(), "Queue ownership mismatch, transfer the ownerships! image: {}, cmd: {}", queue_data_idx_,
                   cmd.queueDataIdx());
}

void Image::copyBuffer(CommandBuffer& cmd, Buffer& src, vec2u extent, vec2i offset) {
    checkQueueOwnership(cmd);

    if (extent.x == 0) {
        extent.x = extent_.x - offset.x;
    }
    if (extent.y == 0) {
        extent.y = extent_.y - offset.y;
    }

    MLE_ASSERT(extent.x > 0 && extent.y > 0);
    MLE_ASSERT(offset.x + extent.x <= extent_.x && offset.y + extent.y <= extent_.y);

    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D{offset.x, offset.y, 0};
    region.imageExtent = toVkExtent3D(extent);

    transitionState(cmd, State::TRANSFER_DST);

    cmd().copyBufferToImage(src.get(), o_, vk::ImageLayout::eTransferDstOptimal, region);
}

void Image::copyImage(CommandBuffer& cmd, Image& src, vec2u extent, vec2i src_offset, vec2i dst_offset) {
    checkQueueOwnership(cmd);
    src.checkQueueOwnership(cmd);

    if (extent.x == 0) {
        extent.x = src.getExtent().x - src_offset.x;
    }
    if (extent.y == 0) {
        extent.y = src.getExtent().y - src_offset.y;
    }

    MLE_ASSERT(extent.x > 0 && extent.y > 0);
    MLE_ASSERT(src_offset.x + extent.x <= src.getExtent().x && src_offset.y + extent.y <= src.getExtent().y);
    MLE_ASSERT(dst_offset.x + extent.x <= extent_.x && dst_offset.y + extent.y <= extent_.y);

    vk::ImageCopy2 region{};
    region.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.srcSubresource.mipLevel = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = 1;
    region.srcOffset = vk::Offset3D{src_offset.x, src_offset.y, 0};
    region.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.dstSubresource.mipLevel = 0;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = 1;
    region.dstOffset = vk::Offset3D{dst_offset.x, dst_offset.y, 0};
    region.extent = vk::Extent3D{extent.x, extent.y, 1};

    vk::CopyImageInfo2 copy_info{};
    copy_info.srcImage = src.get();
    copy_info.dstImage = o_;
    copy_info.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
    copy_info.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
    copy_info.setRegions(region);

    src.transitionState(cmd, State::TRANSFER_SRC);
    transitionState(cmd, State::TRANSFER_DST);
    cmd().copyImage2(copy_info);
}

void Image::blitImage(CommandBuffer& cmd, Image& src, Recti src_rect, Recti dst_rect) {
    checkQueueOwnership(cmd);
    src.checkQueueOwnership(cmd);

    if (src_rect.size.x == 0) {
        src_rect.size.x = as<int>(src.getExtent().x) - src_rect.pos.x;
    }
    if (src_rect.size.y == 0) {
        src_rect.size.y = as<int>(src.getExtent().y) - src_rect.pos.y;
    }
    if (dst_rect.size.x == 0) {
        dst_rect.size.x = as<int>(extent_.x) - dst_rect.pos.x;
    }
    if (dst_rect.size.y == 0) {
        dst_rect.size.y = as<int>(extent_.y) - dst_rect.pos.y;
    }

    MLE_ASSERT(src_rect.size.x > 0 && src_rect.size.y > 0);
    MLE_ASSERT(dst_rect.size.x > 0 && dst_rect.size.y > 0);
    MLE_ASSERT(src_rect.pos.x + src_rect.size.x <= as<int>(src.getExtent().x) && src_rect.pos.y + src_rect.size.y <= as<int>(src.getExtent().y));
    MLE_ASSERT(dst_rect.pos.x + dst_rect.size.x <= as<int>(extent_.x) && dst_rect.pos.y + dst_rect.size.y <= as<int>(extent_.y));

    vk::ImageBlit2 blit{};
    blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.srcSubresource.mipLevel = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.srcOffsets[0].x = src_rect.pos.x;
    blit.srcOffsets[0].y = src_rect.pos.y;
    blit.srcOffsets[0].z = 0;
    blit.srcOffsets[1].x = src_rect.pos.x + src_rect.size.x;
    blit.srcOffsets[1].y = src_rect.pos.y + src_rect.size.y;
    blit.srcOffsets[1].z = 1;
    blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.dstSubresource.mipLevel = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;
    blit.dstOffsets[0].x = dst_rect.pos.x;
    blit.dstOffsets[0].y = dst_rect.pos.y;
    blit.dstOffsets[0].z = 0;
    blit.dstOffsets[1].x = dst_rect.pos.x + dst_rect.size.x;
    blit.dstOffsets[1].y = dst_rect.pos.y + dst_rect.size.y;
    blit.dstOffsets[1].z = 1;

    vk::BlitImageInfo2 blit_info{};
    blit_info.srcImage = src.get();
    blit_info.dstImage = o_;
    blit_info.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
    blit_info.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
    blit_info.filter = vk::Filter::eLinear;
    blit_info.setRegions(blit);

    src.transitionState(cmd, State::TRANSFER_SRC);
    transitionState(cmd, State::TRANSFER_DST);
    cmd().blitImage2(blit_info);
}

BufferHnd Image::copyToBufferOTS(vec2u extent, vec2i offset) {
    if (extent.x == 0) {
        extent.x = extent_.x - offset.x;
    }
    if (extent.y == 0) {
        extent.y = extent_.y - offset.y;
    }

    auto& cmd_mgr = Renderer::i().cmdMgr();
    auto cmd = cmd_mgr.getOTS(queue_data_idx_);

    Buffer::CI buffer_ci{};
    buffer_ci.size = as<usize>(extent.x) * extent.y * getChannelCount();
    buffer_ci.usage = vk::BufferUsageFlagBits::eTransferDst;
    buffer_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY_HOST_READ;
    auto buffer = Buffer::createHnd(buffer_ci);

    transitionState(cmd, State::TRANSFER_SRC);

    vk::BufferImageCopy2 region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = (format_ == ImageFormat::DEPTH) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D{offset.x, offset.y, 0};
    region.imageExtent = vk::Extent3D{extent.x, extent.y, 1};

    vk::CopyImageToBufferInfo2 copy_info{};
    copy_info.srcImage = o_;
    copy_info.dstBuffer = buffer->get();
    copy_info.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
    copy_info.setRegions(region);

    cmd().copyImageToBuffer2(copy_info);

    cmd_mgr.submitOTSWait(std::move(cmd));

    return buffer;
}

void Image::clear(CommandBuffer& cmd, vk::ClearColorValue color) {
    checkQueueOwnership(cmd);

    transitionState(cmd, State::TRANSFER_DST);
    vk::ImageSubresourceRange range{};
    range.aspectMask = vk::ImageAspectFlagBits::eColor;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;
    cmd().clearColorImage(o_, vk::ImageLayout::eTransferDstOptimal, color, range);
}

void Image::clear(CommandBuffer& cmd, vk::ClearDepthStencilValue depth) {
    checkQueueOwnership(cmd);

    transitionState(cmd, State::TRANSFER_DST);
    vk::ImageSubresourceRange range{};
    range.aspectMask = vk::ImageAspectFlagBits::eDepth;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;
    cmd().clearDepthStencilImage(o_, vk::ImageLayout::eTransferDstOptimal, depth, range);
}

BufferHnd Image::copyRaw(CommandBuffer& cmd, const RawData& data, vec2i offset) {
    Buffer::CI staging_ci{};
    staging_ci.size = data.pixels.size();
    staging_ci.usage = vk::BufferUsageFlagBits::eTransferSrc;
    staging_ci.allocation_type = Buffer::CI::AllocationType::STAGING;

    MLE_ASSERT(offset.x + data.extent.x <= extent_.x && offset.y + data.extent.y <= extent_.y);
    MLE_ASSERT(data.channels == getChannelCount());

    auto staging = Buffer::createHnd(staging_ci);

    staging->write(data.pixels.data(), data.pixels.size());

    copyBuffer(cmd, *staging, data.extent, offset);

    return staging;
}

Image::RawData Image::readFile(const std::string& path, int desired_channels) {
    MLE_ASSERT_LOG(!path.empty(), "Image path is empty");
    MLE_ASSERT_LOG(std::filesystem::exists(path), "Image file does not exist: {}", path);
    int width = 0, height = 0, channels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, desired_channels);
    MLE_ASSERT_LOG(pixels && width > 0 && height > 0 && channels > 0, "Problem loading image: {}, pixels: {}, w: {}, h: {}, c: {}", path, as<void*>(pixels),
                   width, height, channels);

    if (desired_channels) {
        channels = desired_channels;
    }

    Image::RawData data;
    data.extent = vec2u{as<u32>(width), as<u32>(height)};
    data.channels = channels;
    data.pixels.resize(as<usize>(width) * height * channels);
    std::memcpy(data.pixels.data(), pixels, data.pixels.size());

    stbi_image_free(pixels);
    return data;
}

[[nodiscard]] int Image::getChannelCount() const {
    switch (format_) {
        case ImageFormat::SWAPCHAIN:
        case ImageFormat::TEXTURE_4U:
        case ImageFormat::TEXTURE_4SRGB:
        case ImageFormat::STORAGE_4U8:
        case ImageFormat::COLOR:
        case ImageFormat::GBUF_PARAMS:
            return 4;
        case ImageFormat::NORMALS:
        case ImageFormat::TEXTURE_2U:
            return 2;
        case ImageFormat::DEPTH:
        case ImageFormat::TEXTURE_1U:
        case ImageFormat::STORAGE_F32:
        case ImageFormat::STORAGE_U32:
            return 1;
        default:
            MLE_UNREACHABLE_LOG("Unsupported image format: {}", as<u32>(format_));
    }
};

void Image::transitionLayout(CommandBuffer& cmd, TransitionLayoutInfo info) {
    if (layout_ == info.new_layout) {
        return;
    }

    checkQueueOwnership(cmd);

    vk::ImageMemoryBarrier2 barrier{};
    barrier.oldLayout = layout_;
    barrier.newLayout = info.new_layout;
    barrier.srcStageMask = info.src_stage_mask;
    barrier.srcAccessMask = info.src_access_mask;
    barrier.dstStageMask = info.dst_stage_mask;
    barrier.dstAccessMask = info.dst_access_mask;
    barrier.image = o_;
    barrier.subresourceRange.aspectMask =
        (usage_ & vk::ImageUsageFlagBits::eDepthStencilAttachment) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vk::DependencyInfo dep_info{};
    dep_info.setImageMemoryBarriers(barrier);

    cmd().pipelineBarrier2(dep_info);

    layout_ = barrier.newLayout;
}

constexpr Image::StateProps Image::getStateProps(State state) {
    switch (state) {
        case State::INITIAL:
            return {.layout = vk::ImageLayout::eUndefined, .stage = {}, .access = {}};
        case State::PRESENT_SRC:
            return {.layout = vk::ImageLayout::ePresentSrcKHR, .stage = vk::PipelineStageFlagBits2::eNone, .access = {}};
        case State::TRANSFER_SRC:
            return {
                .layout = vk::ImageLayout::eTransferSrcOptimal, .stage = vk::PipelineStageFlagBits2::eTransfer, .access = vk::AccessFlagBits2::eTransferRead};
        case State::TRANSFER_DST:
            return {
                .layout = vk::ImageLayout::eTransferDstOptimal, .stage = vk::PipelineStageFlagBits2::eTransfer, .access = vk::AccessFlagBits2::eTransferWrite};
        case State::COLOR_ATT:
            return {.layout = vk::ImageLayout::eAttachmentOptimal,
                    .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                    .access = vk::AccessFlagBits2::eColorAttachmentWrite};
        case State::DEPTH_ATT:
            return {.layout = vk::ImageLayout::eAttachmentOptimal,
                    .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                    .access = vk::AccessFlagBits2::eDepthStencilAttachmentWrite};
        case State::FS_READ:
            return {.layout = vk::ImageLayout::eShaderReadOnlyOptimal,
                    .stage = vk::PipelineStageFlagBits2::eFragmentShader,
                    .access = vk::AccessFlagBits2::eShaderRead};
        case State::COMPUTE_RW:
            return {.layout = vk::ImageLayout::eGeneral,
                    .stage = vk::PipelineStageFlagBits2::eComputeShader,
                    .access = vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead};
        case State::COMPUTE_R:
            return {
                .layout = vk::ImageLayout::eReadOnlyOptimal, .stage = vk::PipelineStageFlagBits2::eComputeShader, .access = vk::AccessFlagBits2::eShaderRead};
        default:
            MLE_UNREACHABLE_LOG("Invalid state: {}", state);
            break;
    }
}

void Image::transitionState(CommandBuffer& cmd, State state) {
    if (state == state_) {
        return;
    }

    auto current_state_props = getStateProps(state_);
    auto new_state_props = getStateProps(state);

    TransitionLayoutInfo info = {};
    info.new_layout = new_state_props.layout;
    info.src_stage_mask = current_state_props.stage;
    info.src_access_mask = current_state_props.access;
    info.dst_stage_mask = new_state_props.stage;
    info.dst_access_mask = new_state_props.access;

    transitionLayout(cmd, info);

    state_ = state;
}

void Image::ownershipRelease(CommandBuffer& cmd, QueueDataIdx dst_queue_data_idx) {
    if (dst_queue_data_idx == queue_data_idx_) {
        return;
    }

    vk::ImageMemoryBarrier2 mb{};
    mb.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    mb.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead;
    mb.dstStageMask = vk::PipelineStageFlagBits2::eNone;
    mb.dstAccessMask = vk::AccessFlagBits2::eNone;
    mb.srcQueueFamilyIndex = Renderer::i().cmdMgr().queueFamilyIdx(queue_data_idx_);
    mb.dstQueueFamilyIndex = Renderer::i().cmdMgr().queueFamilyIdx(dst_queue_data_idx);
    mb.image = o_;
    mb.subresourceRange.aspectMask =
        (usage_ & vk::ImageUsageFlagBits::eDepthStencilAttachment) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    mb.subresourceRange.baseMipLevel = 0;
    mb.subresourceRange.levelCount = 1;
    mb.subresourceRange.baseArrayLayer = 0;
    mb.subresourceRange.layerCount = 1;

    vk::DependencyInfo dep{};
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &mb;

    cmd().pipelineBarrier2(dep);

    prev_queue_data_idx_ = queue_data_idx_;
    queue_data_idx_ = INVALID_QUEUE;
}

std::optional<Semaphore> Image::ownershipReleaseOTS(QueueDataIdx dst_queue_data_idx) {
    if (dst_queue_data_idx == queue_data_idx_) {
        return {};
    }

    auto cmd = Renderer::i().cmdMgr().getOTS(queue_data_idx_);

    ownershipRelease(cmd, dst_queue_data_idx);

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

    return semaphore;
}

void Image::ownershipAcquire(CommandBuffer& cmd, vk::PipelineStageFlags2 dst_stage_mask, vk::AccessFlags2 dst_access_mask) {
    MLE_ASSERT_LOG(queue_data_idx_ == INVALID_QUEUE, "Release wasn't called.");
    MLE_ASSERT(prev_queue_data_idx_ != INVALID_QUEUE && prev_queue_data_idx_ != NO_QUEUE);
    auto next_queue_data_idx = cmd.queueDataIdx();

    vk::ImageMemoryBarrier2 mb{};
    mb.srcStageMask = vk::PipelineStageFlagBits2::eNone;
    mb.srcAccessMask = vk::AccessFlagBits2::eNone;
    mb.dstStageMask = dst_stage_mask;
    mb.dstAccessMask = dst_access_mask;
    mb.srcQueueFamilyIndex = Renderer::i().cmdMgr().queueFamilyIdx(prev_queue_data_idx_);
    mb.dstQueueFamilyIndex = Renderer::i().cmdMgr().queueFamilyIdx(next_queue_data_idx);
    mb.image = o_;
    mb.subresourceRange.aspectMask =
        (usage_ & vk::ImageUsageFlagBits::eDepthStencilAttachment) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    mb.subresourceRange.baseMipLevel = 0;
    mb.subresourceRange.levelCount = 1;
    mb.subresourceRange.baseArrayLayer = 0;
    mb.subresourceRange.layerCount = 1;

    vk::DependencyInfo dep{};
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &mb;

    cmd().pipelineBarrier2(dep);

    queue_data_idx_ = next_queue_data_idx;
    prev_queue_data_idx_ = NO_QUEUE;
}

std::optional<Semaphore> Image::ownershipReleaseOTSAcquire(CommandBuffer& cmd, vk::PipelineStageFlags2 dst_stage_mask, vk::AccessFlags2 dst_access_mask) {
    if (queue_data_idx_ == NO_QUEUE) {
        queue_data_idx_ = cmd.queueDataIdx();
        return {};
    }
    if (queue_data_idx_ == cmd.queueDataIdx()) {
        return {};
    }

    auto semaphore = ownershipReleaseOTS(cmd.queueDataIdx());
    ownershipAcquire(cmd, dst_stage_mask, dst_access_mask);
    return semaphore;
}

void Image::ownershipAcquireOTSWait(QueueDataIdx dst_queue_data_idx) {
    if (queue_data_idx_ == NO_QUEUE) {
        queue_data_idx_ = dst_queue_data_idx;
        return;
    }
    if (queue_data_idx_ == dst_queue_data_idx) {
        return;
    }
    MLE_ASSERT_LOG(queue_data_idx_ == INVALID_QUEUE, "Release wasn't called.");

    auto cmd = Renderer::i().cmdMgr().getOTS(dst_queue_data_idx);

    ownershipAcquire(cmd);

    Renderer::i().cmdMgr().submitOTSWait(std::move(cmd));
}

void Image::ownershipReleaseOTSAcquireOTSWait(GCmdType type) {
    MLE_ASSERT(queue_data_idx_ != INVALID_QUEUE);

    auto queue_data_idx = Renderer::i().cmdMgr().queueDataIdx(type);
    if (queue_data_idx_ == NO_QUEUE) {
        queue_data_idx_ = queue_data_idx;
        return;
    }
    if (queue_data_idx_ == queue_data_idx) {
        return;
    }

    auto cmd = Renderer::i().cmdMgr().getOTS(queue_data_idx);

    auto semaphore = ownershipReleaseOTSAcquire(cmd);

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
