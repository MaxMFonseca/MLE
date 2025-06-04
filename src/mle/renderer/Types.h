#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <spdlog/fmt/fmt.h>
#include <vk_mem_alloc.h>

#include <memory>
#include <vulkan/vulkan.hpp>

#include "mle/common/Types.h"
#include "mle/common/math/Types.h"
#include "mle/common/math/Types2D.h"

namespace mle::renderer {
enum class DataType : u8 { F32, U32, I32, VEC2F, VEC3F, VEC4F, MAT4, SAMPLER2D, COUNT };

enum class CmdType : u8 { GRAPHICS, TRANSFER, COMPUTE, INVALID };

namespace detail {
class VkContext;
class FencePool;
class CommandPoolManager;
};  // namespace detail

class Buffer;
using BufferHnd = std::unique_ptr<Buffer>;
using BufferRef = Buffer*;

class Image;
using ImageHnd = std::unique_ptr<Image>;
using ImageRef = Image*;

class ShaderModule;
using ShaderModuleHnd = std::unique_ptr<ShaderModule>;
using ShaderModuleRef = ShaderModule*;

class Pipeline;
using PipelineHnd = std::unique_ptr<Pipeline>;
using PipelineRef = Pipeline*;

class TextureAtlas;
using TextureAtlasHnd = std::unique_ptr<TextureAtlas>;
using TextureAtlasRef = TextureAtlas*;

class Fence;
using FenceHnd = std::unique_ptr<Fence>;
using FenceRef = Fence*;

class CommandPool;
using CommandPoolHnd = std::unique_ptr<CommandPool>;
using CommandPoolRef = CommandPool*;

class CommandBuffer;
using CommandBufferHnd = std::unique_ptr<CommandBuffer>;
using CommandBufferRef = CommandBuffer*;

struct PipelineGetResult {
    PipelineRef pipeline;
    bool is_new;
};

struct CmdPoolData {
    vk::CommandPool o;
    std::vector<vk::CommandBuffer> buffers;
    std::vector<vk::CommandBuffer> secondary_buffers;
};

struct Texture {
    usize idx = max<usize>();
    Rectf rect = {0.0F, 0.0F, 0.0F, 0.0F};
    float aspect_ratio = 0.0F;

    // bool operator==(const Texture& other) const { return idx == other.idx && rect == other.rect && aspect_ratio == other.aspect_ratio; }
};

struct WriteDescriptorSet {
    vk::WriteDescriptorSet write;
    std::vector<vk::DescriptorImageInfo> image_infos;
};

struct AttachmentInfo {
    ImageRef image{};
    vk::ImageView view{};
    vk::AttachmentLoadOp load = vk::AttachmentLoadOp::eLoad;
    vk::AttachmentStoreOp store = vk::AttachmentStoreOp::eStore;
    vk::ClearValue clear_value{};
};
struct RenderingInfo {
    Recti render_area{};
    std::vector<AttachmentInfo> colors;
    AttachmentInfo depth;
};
}  // namespace mle::renderer

namespace fmt {
template <>
struct formatter<vk::Result> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(vk::Result result, FormatContext& ctx) const {
        return format_to(ctx.out(), "{}", vk::to_string(result));
    }
};

template <>
struct formatter<vk::Extent2D> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(vk::Extent2D const& extent, FormatContext& ctx) const {
        return format_to(ctx.out(), "[{}, {}]", extent.width, extent.height);
    }
};

template <>
struct formatter<vk::Format> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(vk::Format format, FormatContext& ctx) const {
        return format_to(ctx.out(), "{}", vk::to_string(format));
    }
};

template <>
struct formatter<mle::renderer::Texture> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::renderer::Texture& texture, FormatContext& ctx) const {
        return format_to(ctx.out(), "[idx: {}, rect: {}, ar: {}]", texture.idx, texture.rect, texture.aspect_ratio);
    }
};
}  // namespace fmt
