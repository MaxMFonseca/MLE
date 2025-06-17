#pragma once

#define VULKAN_HPP_ASSERT(expr) ((void)0)
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <spdlog/fmt/fmt.h>
#include <vk_mem_alloc.h>

#include <memory>
#include <vulkan/vulkan.hpp>  // This must be the only Vulkan header included directly.

#include "mle/common/Assert.h"
#include "mle/common/Types.h"
#include "mle/common/math/Types.h"
#include "mle/common/math/Types2D.h"

namespace mle::renderer {
enum class DataType : u8 { F32, U32, I32, VEC2F, VEC3F, VEC4F, MAT2, MAT4, SAMPLER2D, COUNT, UNKNOWN };

enum class CmdType : u8 { GRAPHICS, TRANSFER, COMPUTE, INVALID };

struct PushConstantField {
    std::string name;
    u64 offset;
    u64 size;
    DataType type;
};

struct PipelineVertexInputState {
    std::vector<vk::VertexInputBindingDescription> binding_descriptions;
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;
    vk::PipelineVertexInputStateCreateInfo ci;
};

namespace detail {
class VkContext;
class FencePool;
class CommandPoolManager;
class FrameRenderer;
class ShaderCache;
};  // namespace detail

class Buffer;
using BufferHnd = std::unique_ptr<Buffer>;
using BufferRef = Buffer*;

class Image;
using ImageHnd = std::unique_ptr<Image>;
using ImageRef = Image*;

class Pipeline;
using PipelineHnd = std::unique_ptr<Pipeline>;
using PipelineRef = Pipeline*;

class Shader;
using ShaderHnd = std::unique_ptr<Shader>;
using ShaderRef = Shader*;

class Fence;
using FenceHnd = std::unique_ptr<Fence>;
using FenceRef = Fence*;

class CommandPool;
using CommandPoolHnd = std::unique_ptr<CommandPool>;
using CommandPoolRef = CommandPool*;

class CommandBuffer;
using CommandBufferHnd = std::unique_ptr<CommandBuffer>;
using CommandBufferRef = CommandBuffer*;

class RenderingThread;
using RenderingThreadHnd = std::unique_ptr<RenderingThread>;
using RenderingThreadRef = RenderingThread*;

struct Texture {
    ImageRef image{};
    u32 idx = 0;
    bool ready = false;
};

struct CmdPoolData {
    vk::CommandPool o;
    std::vector<vk::CommandBuffer> available_buffers;
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
}  // namespace mle::renderer

namespace fmt {
using namespace mle::renderer;  // NOLINT

template <>
struct formatter<DataType> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(DataType v, FormatContext& ctx) const {
        switch (v) {
            case DataType::F32:
                return format_to(ctx.out(), "F32");
            case DataType::U32:
                return format_to(ctx.out(), "U32");
            case DataType::I32:
                return format_to(ctx.out(), "I32");
            case DataType::VEC2F:
                return format_to(ctx.out(), "VEC2F");
            case DataType::VEC3F:
                return format_to(ctx.out(), "VEC3F");
            case DataType::VEC4F:
                return format_to(ctx.out(), "VEC4F");
            case DataType::MAT2:
                return format_to(ctx.out(), "MAT2");
            case DataType::MAT4:
                return format_to(ctx.out(), "MAT4");
            case DataType::SAMPLER2D:
                return format_to(ctx.out(), "SAMPLER2D");
            case DataType::UNKNOWN:
                return format_to(ctx.out(), "UNKNOWN");
            case DataType::COUNT:
                return format_to(ctx.out(), "COUNT");
        }
        MLE_TODO;
    }
};

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
struct formatter<mle::renderer::CmdType> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::renderer::CmdType cmd_type, FormatContext& ctx) const {
        switch (cmd_type) {
            case mle::renderer::CmdType::GRAPHICS:
                return format_to(ctx.out(), "GRAPHICS");
            case mle::renderer::CmdType::TRANSFER:
                return format_to(ctx.out(), "TRANSFER");
            case mle::renderer::CmdType::COMPUTE:
                return format_to(ctx.out(), "COMPUTE");
            case mle::renderer::CmdType::INVALID:
                return format_to(ctx.out(), "INVALID");
        }
        MLE_TODO;
    }
};

// template <>
// struct formatter<mle::renderer::Texture> : formatter<std::string> {
//     template <typename FormatContext>
//     constexpr auto format(const mle::renderer::Texture& texture, FormatContext& ctx) const {
//         return format_to(ctx.out(), "[idx: {}, rect: {}, ar: {}]", texture.idx, texture.rect, texture.aspect_ratio);
//     }
// };
}  // namespace fmt
