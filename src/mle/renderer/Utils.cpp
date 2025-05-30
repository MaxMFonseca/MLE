#include "Utils.h"

#include <utility>

#include "mle/common/Utils.h"

namespace mle::renderer {
vk::Format toVkFormat(DataType type) {
    switch (type) {
        case DataType::F32:
            return vk::Format::eR32Sfloat;
        case DataType::U32:
            return vk::Format::eR32Uint;
        case DataType::I32:
            return vk::Format::eR32Sint;
        case DataType::VEC2F:
            return vk::Format::eR32G32Sfloat;
        case DataType::VEC3F:
            return vk::Format::eR32G32B32Sfloat;
        case DataType::VEC4F:
            return vk::Format::eR32G32B32A32Sfloat;
        default:
            MLE_UNREACHABLE_LOG("Invalid data type: {}", std::to_underlying(type));
    }
}

DataType fromVkFormat(vk::Format format) {
    switch (format) {
        case vk::Format::eR32Sfloat:
            return DataType::F32;
        case vk::Format::eR32Uint:
            return DataType::U32;
        case vk::Format::eR32Sint:
            return DataType::I32;
        case vk::Format::eR32G32Sfloat:
            return DataType::VEC2F;
        case vk::Format::eR32G32B32Sfloat:
            return DataType::VEC3F;
        case vk::Format::eR32G32B32A32Sfloat:
            return DataType::VEC4F;
        default:
            MLE_UNREACHABLE_LOG("Invalid data type: {}", vk::to_string(format));
    }
}

usize typeSize(DataType type) {
    switch (type) {
        case DataType::F32:
            return sizeof(f32);
        case DataType::U32:
            return sizeof(u32);
        case DataType::I32:
            return sizeof(i32);
        case DataType::VEC2F:
            return sizeof(vec2f);
        case DataType::VEC3F:
            return sizeof(vec3f);
        case DataType::VEC4F:
            return sizeof(vec4f);
        case DataType::MAT4:
            return sizeof(mat4f);
        default:
            MLE_UNREACHABLE_LOG("Invalid data type: {}", std::to_underlying(type));
    }
}

usize typeSize(vk::Format format) {
    return typeSize(fromVkFormat(format));
}

usize typeAlign(DataType type) {
    switch (type) {
        case DataType::F32:
        case DataType::U32:
        case DataType::I32:
            return 4;
        case DataType::VEC2F:
            return 8;
        case DataType::VEC3F:
        case DataType::VEC4F:
        case DataType::MAT4:
            return 16;
        default:
            MLE_UNREACHABLE_LOG("Invalid data type: {}", std::to_underlying(type));
    }
}

const char* toGlslString(DataType type) {
    switch (type) {
        case DataType::F32:
            return "float";
        case DataType::U32:
            return "uint";
        case DataType::I32:
            return "int";
        case DataType::VEC2F:
            return "vec2";
        case DataType::VEC3F:
            return "vec3";
        case DataType::VEC4F:
            return "vec4";
        case DataType::MAT4:
            return "mat4";
        case DataType::SAMPLER2D:
            return "sampler2D";
        default:
            MLE_UNREACHABLE_LOG("Invalid data type: {}", std::to_underlying(type));
    }
}

std::vector<vk::PipelineColorBlendAttachmentState> makeDefaultBlendAttachmentStates(usize count) {
    std::vector<vk::PipelineColorBlendAttachmentState> ret;
    ret.resize(count);
    for (auto& attachment : ret) {
        attachment.blendEnable = vk::True;
        attachment.colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        attachment.colorBlendOp = vk::BlendOp::eAdd;
        attachment.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
        attachment.dstAlphaBlendFactor = vk::BlendFactor::eDstAlpha;
        attachment.alphaBlendOp = vk::BlendOp::eMax;
    }
    return ret;
}
}  // namespace mle::renderer
