#pragma once

#include "Types.h"
#include "mle/common/Color.h"
#include "mle/common/Types.h"
#include "mle/common/math/Types.h"

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

std::vector<vk::PipelineColorBlendAttachmentState> makeDefaultBlendAttachmentStates(usize count);

}  // namespace mle::renderer
