#include "Image.h"

#include <stb_image.h>
#include <vulkan/vulkan_core.h>

#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "Renderer.h"
#include "mle/common/Exception.h"
#include "mle/common/Utils.h"

namespace mle::renderer {
Image::Image(const CI& ci) {
    MLE_T("Creating image {}, extent:{}, format:{}", (void*)this, ci.extent, ci.format);
    createImage(ci);
    MLE_T("Created image {}, vk {}", (void*)this, (void*)obj_);
    createDefaultImageView();
}

Image::Image(const CISwapchain& ci) :
    obj_(ci.image),
    extent_(getVk().getSwapchainExtent().width, getVk().getSwapchainExtent().height),
    format_(getVk().getSwapchainFormat()),
    image_usage_(vk::ImageUsageFlagBits::eTransferDst),
    swapchain_(true) {
    MLE_T("Creating image {}, wrapper for swapchain image {}", (void*)this, (void*)ci.image);
}

Image::~Image() {
    MLE_LOG_THIS_T;
    getVk().getDevice().destroyImageView(default_view_);
    if (allocation_) {
        vmaDestroyImage(getVk().getVma(), obj_, allocation_);
    }
}

void Image::createImage(const CI& ci) {
    MLE_ASSERT(ci.extent.x != 0 && ci.extent.y != 0);

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

    VmaAllocationCreateInfo alloc_ci = {};
    alloc_ci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    alloc_ci.priority = 1.0F;
    alloc_ci.requiredFlags = static_cast<VkMemoryPropertyFlags>(ci.required_mem_flags);

    VkImage vk_image = VK_NULL_HANDLE;
    auto create_result = vmaCreateImage(getVk().getVma(), (VkImageCreateInfo*)&image_ci, &alloc_ci,  // NOLINT
                                        &vk_image, &allocation_, &allocation_info_);
    if (create_result != VK_SUCCESS) {
        MLE_THROW(VK_ERROR, "Failed to create image, VkResult: {}", vk::to_string(vk::Result(create_result)));
    }

    obj_ = vk_image;
    extent_ = {image_ci.extent.width, image_ci.extent.height};
    format_ = image_ci.format;
    image_usage_ = image_ci.usage;
}

void Image::createDefaultImageView() {
    default_view_ = createImageView();
}

vk::ImageView Image::createImageView() const noexcept {
    MLE_LOG_THIS_T;

    vk::ImageViewCreateInfo image_view_ci = {};

    image_view_ci.image = obj_;
    image_view_ci.viewType = vk::ImageViewType::e2D;
    image_view_ci.format = format_;
    image_view_ci.components.r = vk::ComponentSwizzle::eIdentity;
    image_view_ci.components.g = vk::ComponentSwizzle::eIdentity;
    image_view_ci.components.b = vk::ComponentSwizzle::eIdentity;
    image_view_ci.components.a = vk::ComponentSwizzle::eIdentity;
    if (image_usage_ & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
        image_view_ci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    } else {
        image_view_ci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    }
    image_view_ci.subresourceRange.baseMipLevel = 0;
    image_view_ci.subresourceRange.levelCount = 1;
    image_view_ci.subresourceRange.baseArrayLayer = 0;
    image_view_ci.subresourceRange.layerCount = 1;

    return getVk().getDevice().createImageView(image_view_ci);
}

BufferHnd Image::update(vk::CommandBuffer cmd, const void* data, vec2i extent, vec2i offset) {
    if (extent.x == 0) {
        extent.x = extent_.x - offset.x;
    }
    if (extent.y == 0) {
        extent.y = extent_.y - offset.y;
    }

    MLE_ASSERT(extent.x > 0 && extent.y > 0);
    MLE_ASSERT(offset.x + extent.x <= extent_.x && offset.y + extent.y <= extent_.y);

    Buffer::CI staging_buffer_ci = {};
    staging_buffer_ci.size = static_cast<u64>(extent.x) * extent.y * getFormatChannelCount(format_);
    staging_buffer_ci.usage = vk::BufferUsageFlagBits::eTransferSrc;
    staging_buffer_ci.allocation_type = Buffer::CI::AllocationType::STAGING;

    BufferHnd staging_buffer = std::make_unique<Buffer>(staging_buffer_ci);
    staging_buffer->update(data);

    update(cmd, staging_buffer.get(), extent, offset);

    return staging_buffer;
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

    cmd.copyBufferToImage(buffer->get(), obj_, vk::ImageLayout::eTransferDstOptimal, region);
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
    blit_info.dstImage = obj_;
    blit_info.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
    blit_info.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
    blit_info.filter = vk::Filter::eLinear;
    blit_info.setRegions(blit);

    src->transitionState(cmd, State::TRANSFER_SRC);
    transitionState(cmd, State::TRANSFER_DST);

    cmd.blitImage2(blit_info);
}

Image::FileInfo Image::readFileInfo(const fs::path& filepath) {
    int width = 0, height = 0, channels = 0;
    fs::path full_path = ResPath::TEXTURES;
    full_path /= filepath;
    int ok = stbi_info(full_path.c_str(), &width, &height, &channels);
    if (!ok) {
        MLE_THROW(FAILED_TO_OPEN_FILE, "Failed to read image info from file: {}", full_path.generic_string());
    }
    return {.extent = {width, height}, .channels = channels};
}

Image::RawData Image::readFile(const fs::path& filepath, int target_channel_count) {
    Image::RawData ret = {};

    int width = 0, height = 0, channels_in_file = 0;

    stbi_uc* pixels = stbi_load(filepath.c_str(), &width, &height, &channels_in_file, target_channel_count);

    if (!pixels) {
        MLE_THROW(FAILED_TO_OPEN_FILE, "Failed to read image from file: {}", filepath.generic_string());
    }

    ret.channels = target_channel_count == 0 ? channels_in_file : target_channel_count;
    ret.extent = {width, height};

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

    MLE_T("Loaded image {}, size: {}, channel_count:{}", filepath.generic_string(), vec2f{width, height}, ret.channels);

    return ret;
}

void Image::transitionLayout(vk::CommandBuffer cmd, TransitionLayoutInfo info) {
    if (current_layout_ == info.new_layout) {
        return;
    }

    vk::ImageMemoryBarrier2KHR barrier = {};
    barrier.image = obj_;
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
                default: {
                    MLE_UNREACHABLE_LOG("Invalid state transition from DEPTH_ATT to {}", state);
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

std::string toString(const Image::State& s) {
    switch (s) {
        case Image::State::INITIAL:
            return "INITIAL";
        case Image::State::TRANSFER_SRC:
            return "TRANSFER_SRC";
        case Image::State::TRANSFER_DST:
            return "TRANSFER_DST";
        case Image::State::COLOR_ATT:
            return "COLOR_ATT";
        case Image::State::PRESENT:
            return "PRESENT";
        case Image::State::DEPTH_ATT:
            return "DEPTH_ATT";
        case Image::State::SHADER_READ:
            return "SHADER_READ";
    }
    MLE_UNREACHABLE_TODO;
}
}  // namespace mle::renderer
