#include "Image.h"

#include <stb_image.h>
#include <vulkan/vulkan_core.h>

#include <typeinfo>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "Renderer.h"
#include "detail/VkContext.h"
#include "mle/common/Assert.h"
#include "mle/common/Utils.h"
#include "mle/core/Core.h"
#include "mle/renderer/Utils.h"

namespace mle::renderer {
ImageHnd Image::createHnd(const CI& ci) {
    auto ret = std::make_unique<Image>();
    ret->init(ci);
    return ret;
}

Image::~Image() {
    if (!o_) {
        return;
    }

    MLE_LOG_THIS_T;

    for (auto v : views_) {
        detail::getDevice().destroy(v);
    }
    if (allocation_) {
        vmaDestroyImage(detail::getVma(), o_, allocation_);
    }
}

void Image::init(const CI& ci) {
    MLE_T("Creating an image. extent: {}, format: {}, usage: {}, hnd: {}", ci.extent, vk::to_string(ci.format), vk::to_string(ci.usage), (void*)ci.o);

    initImage(ci);
}

void Image::initImage(const CI& ci) {
    MLE_ASSERT(ci.extent.x != 0 && ci.extent.y != 0);

    if (!ci.o) {
        vk::ImageCreateInfo image_ci = {};
        image_ci.imageType = vk::ImageType::e2D;
        image_ci.format = ci.format;
        image_ci.extent.width = ci.extent.x;
        image_ci.extent.height = ci.extent.y;
        image_ci.extent.depth = 1;
        image_ci.mipLevels = 1;
        image_ci.arrayLayers = 1;
        image_ci.samples = vk::SampleCountFlagBits::e1;
        image_ci.tiling = vk::ImageTiling::eOptimal;
        image_ci.usage = ci.usage;
        image_ci.initialLayout = vk::ImageLayout::eUndefined;
        image_ci.sharingMode = vk::SharingMode::eExclusive;

        VmaAllocationCreateInfo alloc_ci = {};
        alloc_ci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        alloc_ci.priority = 1.0F;
        alloc_ci.requiredFlags = static_cast<VkMemoryPropertyFlags>(ci.required_mem_flags);

        VkImage vk_image = VK_NULL_HANDLE;
        auto create_result = vmaCreateImage(detail::getVma(), rAs<VkImageCreateInfo*>(&image_ci), &alloc_ci, &vk_image, &allocation_, &allocation_info_);
        if (create_result != VK_SUCCESS) {
            core::unrecoverable("Failed to create image: {}", as<vk::Result>(create_result));
        }
        o_ = vk_image;
    } else {
        swapchain_ = true;
        o_ = ci.o;
    }

    extent_ = {ci.extent.x, ci.extent.y};
    format_ = ci.format;
    image_usage_ = ci.usage;

    if (!swapchain_) {
        MLE_T("Creating default image view for image: {}", (void*)o_);
        [[maybe_unused]] auto _ = createView();
    }
}

usize Image::createView(const ViewCI& ci) {
    MLE_LOG_THIS_T;

    vk::ImageViewCreateInfo image_view_ci = {};

    image_view_ci.image = o_;
    image_view_ci.viewType = vk::ImageViewType::e2D;
    image_view_ci.format = format_;
    image_view_ci.components.r = ci.r;
    image_view_ci.components.g = ci.g;
    image_view_ci.components.b = ci.b;
    image_view_ci.components.a = ci.a;
    if (image_usage_ & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
        image_view_ci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    } else {
        image_view_ci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    }
    image_view_ci.subresourceRange.baseMipLevel = 0;
    image_view_ci.subresourceRange.levelCount = 1;
    image_view_ci.subresourceRange.baseArrayLayer = 0;
    image_view_ci.subresourceRange.layerCount = 1;

    vk::ImageView vkhnd = unwrap(detail::getDevice().createImageView(image_view_ci));
    views_.push_back(vkhnd);
    return views_.size() - 1;
}

[[nodiscard]] vk::ImageView Image::getView(usize id) const {
    MLE_ASSERT(!views_.empty());
    if (id >= views_.size()) {
        MLE_E("Image view id {} is out of range, using default view id 0", id);
        id = 0;
    }
    return views_.at(id);
}

void Image::update(vk::CommandBuffer cmd, BufferRef buffer, vec2i extent, vec2i offset) {
    if (extent.x == 0) {
        extent.x = extent_.x - offset.x;
    }
    if (extent.y == 0) {
        extent.y = extent_.y - offset.y;
    }

    MLE_ASSERT(extent.x > 0 && extent.y > 0);
    MLE_ASSERT(offset.x + extent.x <= extent_.x && offset.y + extent.y <= extent_.y);

    transitionState(cmd, State::TRANSFER_DST);

    vk::BufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D{static_cast<i32>(offset.x), static_cast<i32>(offset.y), 0};
    region.imageExtent = vk::Extent3D{static_cast<u32>(extent.x), static_cast<u32>(extent.y), 1};

    cmd.copyBufferToImage(buffer->get(), o_, vk::ImageLayout::eTransferDstOptimal, region);
}

void Image::updateCopy(vk::CommandBuffer cmd, ImageRef src, Recti src_rect, Recti dst_rect) {
    if (src_rect.size.x == 0) {
        src_rect.size.x = src->getExtent().x - src_rect.pos.x;
    }
    if (src_rect.size.y == 0) {
        src_rect.size.y = src->getExtent().y - src_rect.pos.y;
    }
    if (dst_rect.size.x == 0) {
        dst_rect.size.x = extent_.x - dst_rect.pos.x;
    }
    if (dst_rect.size.y == 0) {
        dst_rect.size.y = extent_.y - dst_rect.pos.y;
    }

    vk::ImageCopy2 region = {};
    region.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.srcSubresource.mipLevel = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = 1;
    region.srcOffset = vk::Offset3D{src_rect.pos.x, src_rect.pos.y, 0};

    region.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.dstSubresource.mipLevel = 0;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = 1;
    region.dstOffset = vk::Offset3D{dst_rect.pos.x, dst_rect.pos.y, 0};

    region.extent = vk::Extent3D{
        static_cast<uint32_t>(src_rect.size.x),
        static_cast<uint32_t>(src_rect.size.y),
        1,
    };

    vk::CopyImageInfo2 copy_info = {};
    copy_info.srcImage = src->get();
    copy_info.dstImage = o_;
    copy_info.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
    copy_info.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
    copy_info.setRegions(region);

    src->transitionState(cmd, State::TRANSFER_SRC);
    transitionState(cmd, State::TRANSFER_DST);

    cmd.copyImage2(copy_info);
}

void Image::updateBlit(vk::CommandBuffer cmd, ImageRef src, Recti src_rect, Recti dst_rect) {
    if (src_rect.size.x == 0) {
        src_rect.size.x = src->getExtent().x - src_rect.pos.x;
    }
    if (src_rect.size.y == 0) {
        src_rect.size.y = src->getExtent().y - src_rect.pos.y;
    }
    if (dst_rect.size.x == 0) {
        dst_rect.size.x = extent_.x - dst_rect.pos.x;
    }
    if (dst_rect.size.y == 0) {
        dst_rect.size.y = extent_.y - dst_rect.pos.y;
    }

    vk::ImageBlit2 blit = {};
    blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.srcSubresource.layerCount = 1;
    blit.srcOffsets[0].x = src_rect.pos.x;
    blit.srcOffsets[0].y = src_rect.pos.y;
    blit.srcOffsets[0].z = 0;
    blit.srcOffsets[1].x = src_rect.pos.x + src_rect.size.x;
    blit.srcOffsets[1].y = src_rect.pos.y + src_rect.size.y;
    blit.srcOffsets[1].z = 1;
    blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.dstSubresource.layerCount = 1;
    blit.dstOffsets[0].x = dst_rect.pos.x;
    blit.dstOffsets[0].y = dst_rect.pos.y;
    blit.dstOffsets[0].z = 0;
    blit.dstOffsets[1].x = dst_rect.pos.x + dst_rect.size.x;
    blit.dstOffsets[1].y = dst_rect.pos.y + dst_rect.size.y;
    blit.dstOffsets[1].z = 1;

    vk::BlitImageInfo2 blit_info = {};
    blit_info.srcImage = src->get();
    blit_info.dstImage = o_;
    blit_info.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
    blit_info.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
    blit_info.filter = vk::Filter::eLinear;
    blit_info.setRegions(blit);

    src->transitionState(cmd, State::TRANSFER_SRC);
    transitionState(cmd, State::TRANSFER_DST);

    cmd.blitImage2(blit_info);
}

Image::FileInfo Image::readFileInfo(const std::string& path) {
    int width = 0, height = 0, channels = 0;
    int ok = stbi_info(path.c_str(), &width, &height, &channels);
    if (!ok) {
        core::unrecoverable("Failed to read image info from file: {}", path);
    }
    return {.extent = {width, height}, .channels = channels};
}

Image::RawData Image::readFile(const std::string& path, int target_channel_count) {
    Image::RawData ret = {};

    int width = 0, height = 0, channels_in_file = 0;

    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels_in_file, target_channel_count);

    if (!pixels) {
        core::unrecoverable("Failed to read image from file: {}", path);
    }

    ret.channels = target_channel_count == 0 ? channels_in_file : target_channel_count;
    ret.extent = {width, height};

    // FIXME: THIS... this is stupid
    if (ret.channels == 3) {
        ret.channels = 4;
        ret.pixels.resize(width * height * 4);  // NOLINT
        for (int i = 0; i < width * height; i++) {
            ret.pixels[i * 4 + 0] = pixels[i * 3 + 0];  // NOLINT
            ret.pixels[i * 4 + 1] = pixels[i * 3 + 1];  // NOLINT
            ret.pixels[i * 4 + 2] = pixels[i * 3 + 2];  // NOLINT
            ret.pixels[i * 4 + 3] = 0xFF;               // NOLINT
        }
    } else {
        ret.pixels.resize(static_cast<usize>(width) * height * ret.channels);
        memcpy(ret.pixels.data(), pixels, ret.pixels.size());
    }

    stbi_image_free(pixels);

    MLE_T("Loaded image {}, size: {}, channel_count:{}", path, vec2f{width, height}, ret.channels);

    return ret;
}

BufferHnd Image::createStagingBuffer(const void* data, vec2i extent, int channels) {
    Buffer::CI staging_buffer_ci = {};
    staging_buffer_ci.size = as<u64>(extent.x) * extent.y * channels;
    staging_buffer_ci.usage = vk::BufferUsageFlagBits::eTransferSrc;
    staging_buffer_ci.allocation_type = Buffer::CI::AllocationType::STAGING;

    auto staging_buffer = Buffer::createHnd(staging_buffer_ci);
    staging_buffer->update(data);

    return staging_buffer;
}

void Image::transitionLayout(vk::CommandBuffer cmd, TransitionLayoutInfo info) {
    if (current_layout_ == info.new_layout) {
        return;
    }

    vk::ImageMemoryBarrier2 barrier = {};
    barrier.image = o_;
    barrier.oldLayout = current_layout_;
    barrier.newLayout = info.new_layout;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    if (image_usage_ & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    } else {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    barrier.srcStageMask = info.src_stage_mask;
    barrier.srcAccessMask = info.src_access_mask;
    barrier.dstStageMask = info.dst_stage_mask;
    barrier.dstAccessMask = info.dst_access_mask;

    vk::DependencyInfo dependency_info = {};
    dependency_info.setImageMemoryBarriers(barrier);

    cmd.pipelineBarrier2(dependency_info);

    current_layout_ = barrier.newLayout;
}

void Image::changeOwnerQueue(CmdType curr, vk::CommandBuffer curr_cmd, CmdType next, vk::CommandBuffer next_cmd) {
    if (curr == next) {
        MLE_ASSERT_LOG(false, "Cannot change owner queue to the same queue");
        return;
    }

    auto src_family = detail::getVk().getQueueIndex(curr);
    auto dst_family = detail::getVk().getQueueIndex(next);

    vk::ImageMemoryBarrier2KHR barrier = {};
    barrier.image = o_;
    barrier.oldLayout = current_layout_;
    barrier.newLayout = current_layout_;
    barrier.srcQueueFamilyIndex = static_cast<u32>(src_family);
    barrier.dstQueueFamilyIndex = static_cast<u32>(dst_family);
    barrier.subresourceRange.aspectMask = (image_usage_ & vk::ImageUsageFlagBits::eDepthStencilAttachment)
                                              ? (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)
                                              : vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    barrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    barrier.dstAccessMask = vk::AccessFlagBits2::eMemoryRead;

    vk::DependencyInfo dep_info = {};
    dep_info.setImageMemoryBarriers(barrier);

    curr_cmd.pipelineBarrier2(dep_info);
    next_cmd.pipelineBarrier2(dep_info);
}

void Image::transitionState(vk::CommandBuffer cmd, State state) {
    if (state == current_state_) {
        return;
    }

    TransitionLayoutInfo info = {};

    switch (current_state_) {
        case State::INITIAL: {
            switch (state) {
                case State::TRANSFER_DST: {
                    info.new_layout = vk::ImageLayout::eTransferDstOptimal;
                    if (swapchain_) {
                        info.src_stage_mask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                        info.src_access_mask = {};
                    } else {
                        info.src_stage_mask = {};
                        info.src_access_mask = {};
                    }
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eTransfer;
                    info.dst_access_mask = vk::AccessFlagBits2::eTransferWrite;
                } break;
                case State::COLOR_ATT: {
                    info.new_layout = vk::ImageLayout::eAttachmentOptimal;
                    info.src_stage_mask = {};
                    info.src_access_mask = {};
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                    info.dst_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
                } break;
                case State::DEPTH_ATT: {
                    info.new_layout = vk::ImageLayout::eAttachmentOptimal;
                    info.src_stage_mask = {};
                    info.src_access_mask = {};
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eEarlyFragmentTests;
                    info.dst_access_mask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
                } break;
                case State::COMPUTE_RW: {
                    info.new_layout = vk::ImageLayout::eGeneral;
                    info.src_stage_mask = {};
                    info.src_access_mask = {};
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eComputeShader;
                    info.dst_access_mask = vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
                } break;
                default: {
                    MLE_UNREACHABLE_LOG("Invalid state transition from INITIAL to {}", state);
                } break;
            }
        } break;
        case State::TRANSFER_SRC: {
            switch (state) {
                case State::TRANSFER_DST: {
                    info.new_layout = vk::ImageLayout::eTransferDstOptimal;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eTransfer;
                    info.src_access_mask = vk::AccessFlagBits2::eTransferRead;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eTransfer;
                    info.dst_access_mask = vk::AccessFlagBits2::eTransferWrite;
                } break;
                case State::COLOR_ATT: {
                    info.new_layout = vk::ImageLayout::eAttachmentOptimal;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eTransfer;
                    info.src_access_mask = vk::AccessFlagBits2::eTransferRead;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                    info.dst_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
                } break;
                default: {
                    MLE_UNREACHABLE_LOG("Invalid state transition from TRANSFER_SRC to {}", state);
                } break;
            }
        } break;
        case State::TRANSFER_DST: {
            switch (state) {
                case State::PRESENT: {
                    info.new_layout = vk::ImageLayout::ePresentSrcKHR;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eTransfer;
                    info.src_access_mask = vk::AccessFlagBits2::eTransferWrite;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eNone;
                    info.dst_access_mask = {};
                } break;
                case State::COLOR_ATT: {
                    info.new_layout = vk::ImageLayout::eAttachmentOptimal;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eTransfer;
                    info.src_access_mask = vk::AccessFlagBits2::eTransferWrite;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                    info.dst_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
                } break;
                case State::SHADER_READ: {
                    info.new_layout = vk::ImageLayout::eReadOnlyOptimal;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eTransfer;
                    info.src_access_mask = vk::AccessFlagBits2::eTransferWrite;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eFragmentShader;
                    info.dst_access_mask = vk::AccessFlagBits2::eShaderRead;
                } break;
                default: {
                    MLE_UNREACHABLE_LOG("Invalid state transition from TRANSFER_DST to {}", state);
                } break;
            }
        } break;
        case State::COLOR_ATT: {
            switch (state) {
                case State::TRANSFER_SRC: {
                    info.new_layout = vk::ImageLayout::eTransferSrcOptimal;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                    info.src_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eTransfer;
                    info.dst_access_mask = vk::AccessFlagBits2::eTransferRead;
                } break;
                case State::TRANSFER_DST: {
                    info.new_layout = vk::ImageLayout::eTransferDstOptimal;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                    info.src_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eTransfer;
                    info.dst_access_mask = vk::AccessFlagBits2::eTransferWrite;
                } break;
                case State::SHADER_READ: {
                    info.new_layout = vk::ImageLayout::eReadOnlyOptimal;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                    info.src_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eFragmentShader;
                    info.dst_access_mask = vk::AccessFlagBits2::eShaderRead;
                } break;
                default: {
                    MLE_UNREACHABLE_LOG("Invalid state transition from COLOR_ATT to {}", state);
                } break;
            }
        } break;
        case State::PRESENT: {
            switch (state) {
                case State::TRANSFER_DST: {
                    info.new_layout = vk::ImageLayout::eTransferDstOptimal;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                    info.src_access_mask = {};
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eTransfer;
                    info.dst_access_mask = vk::AccessFlagBits2::eTransferWrite;
                } break;
                default: {
                    MLE_UNREACHABLE_LOG("Invalid state transition from PRESENT to {}", state);
                } break;
            }
        } break;
        case State::DEPTH_ATT: {
            switch (state) {
                case State::SHADER_READ: {
                    info.new_layout = vk::ImageLayout::eReadOnlyOptimal;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eEarlyFragmentTests;
                    info.src_access_mask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eFragmentShader;
                    info.dst_access_mask = vk::AccessFlagBits2::eShaderRead;
                } break;
                default: {
                    MLE_UNREACHABLE_LOG("Invalid state transition from DEPTH_ATT to {}", state);
                } break;
            }
        } break;
        case State::COMPUTE_RW: {
            switch (state) {
                case State::TRANSFER_SRC: {
                    info.new_layout = vk::ImageLayout::eTransferDstOptimal;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eComputeShader;
                    info.src_access_mask = vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eTransfer;
                    info.dst_access_mask = vk::AccessFlagBits2::eTransferRead;
                } break;
                case State::SHADER_READ: {
                    info.new_layout = vk::ImageLayout::eReadOnlyOptimal;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eComputeShader;
                    info.src_access_mask = vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eFragmentShader;
                    info.dst_access_mask = vk::AccessFlagBits2::eShaderRead;
                } break;
                default: {
                    MLE_UNREACHABLE_LOG("Invalid state transition from COMPUTE to {}", state);
                } break;
            }
        } break;
        case State::SHADER_READ: {
            switch (state) {
                case State::TRANSFER_DST: {
                    info.new_layout = vk::ImageLayout::eTransferDstOptimal;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eFragmentShader;
                    info.src_access_mask = vk::AccessFlagBits2::eShaderRead;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eTransfer;
                    info.dst_access_mask = vk::AccessFlagBits2::eTransferWrite;
                } break;
                case State::COLOR_ATT: {
                    info.new_layout = vk::ImageLayout::eAttachmentOptimal;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eFragmentShader;
                    info.src_access_mask = vk::AccessFlagBits2::eShaderRead;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                    info.dst_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
                } break;
                case State::COMPUTE_RW: {
                    info.new_layout = vk::ImageLayout::eGeneral;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eFragmentShader;
                    info.src_access_mask = vk::AccessFlagBits2::eShaderRead;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eComputeShader;
                    info.dst_access_mask = vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
                } break;
                case State::DEPTH_ATT: {
                    info.new_layout = vk::ImageLayout::eAttachmentOptimal;
                    info.src_stage_mask = vk::PipelineStageFlagBits2::eFragmentShader;
                    info.src_access_mask = vk::AccessFlagBits2::eShaderRead;
                    info.dst_stage_mask = vk::PipelineStageFlagBits2::eEarlyFragmentTests;
                    info.dst_access_mask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
                } break;
                default: {
                    MLE_UNREACHABLE_LOG("Invalid state transition from SHADER_READ to {}", state);
                } break;
            }
        }
    }

    transitionLayout(cmd, info);

    current_state_ = state;
}

int Image::getFormatChannelCount(vk::Format format) {
    switch (format) {
        case vk::Format::eR8G8B8A8Unorm:
        case vk::Format::eB8G8R8A8Unorm:
        case vk::Format::eB8G8R8A8Srgb:
        case vk::Format::eR8G8B8A8Srgb:
            return 4;
        case vk::Format::eR8G8B8Unorm:
        case vk::Format::eB8G8R8Unorm:
            return 3;
        case vk::Format::eR8Unorm:
        case vk::Format::eR8Snorm:
            return 1;
        default:
            break;
    }
    MLE_UNREACHABLE_LOG("Unsupported format: {}", vk::to_string(format));
}

vk::Format Image::getDefaultFormatForChannelCount(int c) {
    switch (c) {
        case 1:
            return vk::Format::eR8Unorm;
        case 2:
            return vk::Format::eR8G8Unorm;
        case 3:
        case 4:
            return vk::Format::eR8G8B8A8Srgb;
        default:
            std::unreachable();
            break;
    }
}

u64 Image::getAllocationSize() const {
    return allocation_info_.size;
}

u64 Image::getSizeInBytes() const {
    return static_cast<u64>(extent_.x) * extent_.y * getFormatChannelCount(format_);
}

}  // namespace mle::renderer
