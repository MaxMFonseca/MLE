#pragma once

#include "Types.h"
#include "mle/common/Assert.h"
#include "mle/common/Color.h"
#include "mle/common/Types.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types.h"
#include "mle/core/Core.h"

namespace mle::renderer {
inline vk::Extent2D toVkExtent2D(const vec2i& extent) {
    return {static_cast<u32>(extent.x), static_cast<u32>(extent.y)};
}

usize typeSize(DataType type);
usize typeSize(vk::Format format);
usize typeAlign(DataType type);
vk::Format toVkFormat(DataType type);
DataType fromVkFormat(vk::Format format);
const char* toGlslString(DataType type);

inline vk::ClearColorValue toVkClearColor(const Color& color) {
    vk::ClearColorValue ret;
    ret.setFloat32({color.r, color.g, color.b, color.a});
    return ret;
}

std::vector<vk::PipelineColorBlendAttachmentState> makeDefaultBlendAttachmentStates(usize count, bool alpha = true);

inline bool isVkError(vk::Result result) {
    return as<i64>(result) < 0;
}

inline void check(vk::Result result) {
    if (isVkError(result)) {
        core::unrecoverable("Vulkan error: {}", result);
    }
}

template <typename T>
[[nodiscard]] T unwrap(vk::ResultValue<T>&& rv) {
    check(vk::Result(rv.result));
    return std::move(rv).value;
}

constexpr u64 sizeOf(DataType t) {
    switch (t) {
        case DataType::I32:
            return sizeof(i32);
        case DataType::U32:
            return sizeof(u32);
        case DataType::F32:
            return sizeof(f32);
        case DataType::VEC2F:
            return sizeof(vec2f);
        case DataType::VEC3F:
            return sizeof(vec3f);
        case DataType::VEC4F:
            return sizeof(vec4f);
        case DataType::MAT2:
            return sizeof(mat2f);
        case DataType::MAT4:
            return sizeof(mat4f);
        case DataType::SAMPLER2D:
        case DataType::UNKNOWN:
        case DataType::COUNT:
            void(0);
    }
    MLE_UNREACHABLE_LOG("Unsuported sizeof data type {}", t);
}

constexpr u64 alignUp(u64 value, u64 alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}
}  // namespace mle::renderer
