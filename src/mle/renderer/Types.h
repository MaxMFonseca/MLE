#pragma once

#include "mle/core/Assert.h"

#define VULKAN_HPP_ASSERT(expr) ((void)0)
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_EXCEPTIONS

#include <vk_mem_alloc.h>

#include <vulkan/vulkan.hpp>

#include "mle/math/Types.h"
#include "mle/utils/Types.h"

namespace mle {
class Renderer;
class VkCtx;
class RendererCommandManager;

class Image;
using ImageHnd = std::unique_ptr<Image>;
using ImageRef = Image*;

class Buffer;
using BufferHnd = std::unique_ptr<Buffer>;
using BufferRef = Buffer*;

class Shader;
using ShaderHnd = std::unique_ptr<Shader>;
using ShaderRef = Shader*;

class CommandBuffer;
using CommandBufferHnd = std::unique_ptr<CommandBuffer>;
using CommandBufferRef = CommandBuffer*;

class RendererCommandManager;
class ResetCommandPool;

class Pipeline;
using PipelineHnd = std::unique_ptr<Pipeline>;
using PipelineRef = Pipeline*;

class TextureAtlas;
using TextureAtlasHnd = std::unique_ptr<TextureAtlas>;
using TextureAtlasRef = TextureAtlas*;

class Font;
using FontHnd = std::unique_ptr<Font>;
using FontRef = Font*;

class PipelineCache;

enum class GCmdType : u8 { GRAPHICS = 0, COMPUTE = 1, TRANSFER = 2, G = GRAPHICS, C = COMPUTE, T = TRANSFER };

using QueueDataIdx = u32;
constexpr QueueDataIdx NO_QUEUE = max<QueueDataIdx>() - 1;
constexpr QueueDataIdx INVALID_QUEUE = max<QueueDataIdx>();

enum class ImageFormat : u8 {
    DEPTH,
    TEXTURE_4U,
    TEXTURE_4SRGB,
    TEXTURE_2U,
    TEXTURE_1U,
    GBUF_PARAMS,
    NORMALS,
    COLOR,
    STORAGE_4U8,
    STORAGE_F32,
    STORAGE_U32,
    COUNT,
    SWAPCHAIN
};

constexpr u64 DEFAULT_TIMEOUT_NS = 1'000'000'000;

struct BufferSlice {
    BufferRef buffer{};
    vk::DeviceSize size = 0;
    vk::DeviceSize offset = 0;
};

struct AttachmentInfo {
    ImageRef image{};
    vk::ImageView view{};
    vk::AttachmentLoadOp load_op = vk::AttachmentLoadOp::eLoad;
    vk::AttachmentStoreOp store_op = vk::AttachmentStoreOp::eStore;
    vk::ClearValue clear_value{};
};

}  // namespace mle

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
struct formatter<mle::GCmdType> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::GCmdType type, FormatContext& ctx) const {
        switch (type) {
            case mle::GCmdType::GRAPHICS:
                return format_to(ctx.out(), "GRAPHICS");
            case mle::GCmdType::COMPUTE:
                return format_to(ctx.out(), "COMPUTE");
            case mle::GCmdType::TRANSFER:
                return format_to(ctx.out(), "TRANSFER");
            default:
                return format_to(ctx.out(), "UNKNOWN");
        }
    }
};

template <>
struct formatter<mle::ImageFormat> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::ImageFormat format, FormatContext& ctx) const {
        switch (format) {
            case mle::ImageFormat::DEPTH:
                return format_to(ctx.out(), "DEPTH");
            case mle::ImageFormat::TEXTURE_4U:
                return format_to(ctx.out(), "TEXTURE_4U");
            case mle::ImageFormat::TEXTURE_4SRGB:
                return format_to(ctx.out(), "TEXTURE_4SRGB");
            case mle::ImageFormat::TEXTURE_2U:
                return format_to(ctx.out(), "TEXTURE_2U");
            case mle::ImageFormat::TEXTURE_1U:
                return format_to(ctx.out(), "TEXTURE_1U");
            case mle::ImageFormat::GBUF_PARAMS:
                return format_to(ctx.out(), "GBUF_PARAMS");
            case mle::ImageFormat::NORMALS:
                return format_to(ctx.out(), "NORMALS");
            case mle::ImageFormat::COLOR:
                return format_to(ctx.out(), "COLOR");
            case mle::ImageFormat::STORAGE_4U8:
                return format_to(ctx.out(), "STORAGE_4U8");
            case mle::ImageFormat::STORAGE_F32:
                return format_to(ctx.out(), "STORAGE_F32");
            case mle::ImageFormat::STORAGE_U32:
                return format_to(ctx.out(), "STORAGE_U32");
            case mle::ImageFormat::SWAPCHAIN:
                return format_to(ctx.out(), "SWAPCHAIN");
            case mle::ImageFormat::COUNT:
                return format_to(ctx.out(), "COUNT");
            default:
                MLE_UNREACHABLE_LOG("Unknown ImageFormat: {}", as<mle::u8>(format));
        }
    }
};

template <>

struct formatter<vk::DescriptorPoolSize> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const vk::DescriptorPoolSize& size, FormatContext& ctx) const {
        return format_to(ctx.out(), "{{type: {}, count: {}}}", vk::to_string(size.type), size.descriptorCount);
    }
};

}  // namespace fmt
