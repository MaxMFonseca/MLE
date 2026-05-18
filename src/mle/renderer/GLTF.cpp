#include "GLTF.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "mle/core/Assert.h"

namespace mle {

Result GLTF::load(const Path& path) {
    if (!std::filesystem::exists(path)) {
        MLE_E("GLTF file does not exist: {}", path);
        return Result::FAILED_TO_OPEN;
    }

    if (path.extension() != ".glb") {
        MLE_E("Only binary GLB model files are supported: {}", path);
        return Result::FAILED_TO_OPEN;
    }

    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    std::error_code ec;
    source_path_ = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        source_path_ = path.lexically_normal();
    }

    bool ok = false;

    ok = loader.LoadBinaryFromFile(&model_, &err, &warn, path);

    if (!warn.empty()) {
        MLE_W("GLTF warning: {}", warn);
    }
    if (!ok) {
        MLE_E("Failed to load GLTF file: {}", err);
        return Result::FAILED_TO_OPEN;
    }

    return Result::OK;
}

const tinygltf::Accessor& GLTF::getAccessor(int idx) const {
    MLE_ASSERT_LOG(idx >= 0 && idx < as<int>(model_.accessors.size()), "Accessor index out of range");
    return model_.accessors[as<usize>(idx)];
}

const tinygltf::BufferView& GLTF::getBufferView(const tinygltf::Accessor& acc) const {
    MLE_ASSERT_LOG(acc.bufferView >= 0 && acc.bufferView < as<int>(model_.bufferViews.size()), "Accessor without valid bufferView");
    return model_.bufferViews[as<usize>(acc.bufferView)];
}

const tinygltf::Buffer& GLTF::getBuffer(const tinygltf::BufferView& view) const {
    MLE_ASSERT_LOG(view.buffer >= 0 && view.buffer < as<int>(model_.buffers.size()), "BufferView without valid buffer");
    return model_.buffers[as<usize>(view.buffer)];
}

std::vector<vec2f> GLTF::readAccessorVec2f(const tinygltf::Accessor& accessor) const {
    std::vector<vec2f> out;

    const auto& view = getBufferView(accessor);
    const auto& buffer = getBuffer(view);

    MLE_ASSERT_LOG(accessor.type == TINYGLTF_TYPE_VEC2, "Expected VEC2 accessor");
    MLE_ASSERT_LOG(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expected FLOAT component type for VEC2 accessor");

    const usize count = accessor.count;
    const usize stride = accessor.ByteStride(view);
    const usize elem_size = sizeof(float) * 2;
    const usize byte_start = as<usize>(view.byteOffset) + as<usize>(accessor.byteOffset);

    MLE_ASSERT_LOG(stride == 0 || stride >= elem_size, "Invalid stride for VEC2 accessor");

    out.resize(count);

    const auto* base = buffer.data.data() + byte_start;
    const usize step = (stride != 0) ? stride : elem_size;

    for (usize i = 0; i < count; ++i) {
        const auto* p = rAs<const float*>(base + (i * step));
        out[i] = vec2f{p[0], p[1]};
    }

    return out;
}

std::vector<vec4f> GLTF::readAccessorVec4f(const tinygltf::Accessor& accessor) const {
    std::vector<vec4f> out;

    const auto& view = getBufferView(accessor);
    const auto& buffer = getBuffer(view);

    MLE_ASSERT_LOG(accessor.type == TINYGLTF_TYPE_VEC4, "Expected VEC4 accessor");
    MLE_ASSERT_LOG(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expected FLOAT component type for VEC4 accessor");

    const usize count = accessor.count;
    const usize stride = accessor.ByteStride(view);
    const usize elem_size = sizeof(float) * 4;
    const usize byte_start = as<usize>(view.byteOffset) + as<usize>(accessor.byteOffset);

    MLE_ASSERT_LOG(stride == 0 || stride >= elem_size, "Invalid stride for VEC4 accessor");

    out.resize(count);

    const auto* base = buffer.data.data() + byte_start;
    const usize step = (stride != 0) ? stride : elem_size;

    for (usize i = 0; i < count; ++i) {
        const auto* p = rAs<const float*>(base + (i * step));
        out[i] = vec4f{p[0], p[1], p[2], p[3]};
    }

    return out;
}

std::vector<vec3f> GLTF::readAccessorVec3f(const tinygltf::Accessor& accessor) const {
    std::vector<vec3f> out;

    const auto& view = getBufferView(accessor);
    const auto& buffer = getBuffer(view);

    MLE_ASSERT_LOG(accessor.type == TINYGLTF_TYPE_VEC3, "Expected VEC3 accessor");
    MLE_ASSERT_LOG(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expected FLOAT component type for VEC3 accessor");

    const usize count = accessor.count;
    const usize stride = accessor.ByteStride(view);
    const usize elem_size = sizeof(float) * 3;
    const usize byte_start = as<usize>(view.byteOffset) + as<usize>(accessor.byteOffset);

    MLE_ASSERT_LOG(stride == 0 || stride >= elem_size, "Invalid stride for VEC3 accessor");

    out.resize(count);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    const auto* base = buffer.data.data() + byte_start;
    const usize step = (stride != 0) ? stride : elem_size;

    for (usize i = 0; i < count; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
        const auto* p = rAs<const float*>(base + (i * step));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
        out[i] = vec3f{p[0], p[1], p[2]};
    }

    return out;
}

Image::RawData GLTF::readImageRawData(usize image_index, bool srgb) const {
    MLE_ASSERT_LOG(image_index < model_.images.size(), "Image index out of range");

    const auto& image = model_.images.at(image_index);
    MLE_ASSERT_LOG(image.width > 0 && image.height > 0, "Invalid GLTF image extent");
    MLE_ASSERT_LOG(image.component > 0, "Invalid GLTF image channel count");
    MLE_ASSERT_LOG(image.bits == 8 || image.bits == -1, "Only 8-bit GLTF images are supported");

    Image::RawData out{};
    out.extent = vec2u{as<u32>(image.width), as<u32>(image.height)};
    out.channels = 4;
    out.srgb = srgb;
    out.pixels.resize(as<usize>(out.extent.x) * out.extent.y * out.channels);

    const usize src_channels = as<usize>(image.component);
    const usize pixel_count = as<usize>(out.extent.x) * out.extent.y;
    MLE_ASSERT_LOG(image.image.size() >= pixel_count * src_channels, "GLTF image data is smaller than expected");

    for (usize i = 0; i < pixel_count; ++i) {
        const usize src = i * src_channels;
        const usize dst = i * 4;
        out.pixels[dst + 0] = image.image[src + 0];
        out.pixels[dst + 1] = src_channels > 1 ? image.image[src + 1] : image.image[src + 0];
        out.pixels[dst + 2] = src_channels > 2 ? image.image[src + 2] : image.image[src + 0];
        out.pixels[dst + 3] = src_channels > 3 ? image.image[src + 3] : 255;
    }

    return out;
}

std::vector<vec3f> GLTF::readAccessorColor3f(const tinygltf::Accessor& accessor) const {
    std::vector<vec3f> out;

    const auto& view = getBufferView(accessor);
    const auto& buffer = getBuffer(view);

    MLE_ASSERT_LOG(accessor.type == TINYGLTF_TYPE_VEC3 || accessor.type == TINYGLTF_TYPE_VEC4, "COLOR_0 accessor must be VEC3 or VEC4");
    MLE_ASSERT_LOG(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT || accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
                       accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
                   "Unsupported COLOR_0 component type");

    const usize component_count = accessor.type == TINYGLTF_TYPE_VEC4 ? 4UL : 3UL;
    const usize component_size = [&]() {
        switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                return sizeof(float);
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                return sizeof(u8);
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                return sizeof(u16);
            default:
                MLE_UNREACHABLE;
        }
    }();

    const usize count = accessor.count;
    const usize stride = accessor.ByteStride(view);
    const usize elem_size = component_size * component_count;
    const usize byte_start = as<usize>(view.byteOffset) + as<usize>(accessor.byteOffset);

    MLE_ASSERT_LOG(stride == 0 || stride >= elem_size, "Invalid stride for COLOR_0 accessor");

    out.resize(count);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    const auto* base = buffer.data.data() + byte_start;
    const usize step = (stride != 0) ? stride : elem_size;

    switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_FLOAT: {
            for (usize i = 0; i < count; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
                const auto* p = rAs<const float*>(base + (i * step));
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
                out[i] = vec3f{p[0], p[1], p[2]};
            }
        } break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            constexpr float INV_MAX = 1.0F / 255.0F;
            for (usize i = 0; i < count; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
                const auto* p = rAs<const u8*>(base + (i * step));
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
                out[i] = vec3f{as<f32>(p[0]) * INV_MAX, as<f32>(p[1]) * INV_MAX, as<f32>(p[2]) * INV_MAX};
            }
        } break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            constexpr float INV_MAX = 1.0F / 65535.0F;
            for (usize i = 0; i < count; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
                const auto* p = rAs<const u16*>(base + (i * step));
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
                out[i] = vec3f{as<f32>(p[0]) * INV_MAX, as<f32>(p[1]) * INV_MAX, as<f32>(p[2]) * INV_MAX};
            }
        } break;
        default:
            MLE_UNREACHABLE;
    }

    return out;
}

std::vector<quat> GLTF::readAccessorQuat(const tinygltf::Accessor& accessor) const {
    std::vector<quat> out;

    const auto& view = getBufferView(accessor);
    const auto& buffer = getBuffer(view);

    MLE_ASSERT_LOG(accessor.type == TINYGLTF_TYPE_VEC4, "Expected VEC4 accessor");
    MLE_ASSERT_LOG(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expected FLOAT component type for VEC4 accessor");

    const usize count = accessor.count;
    const usize stride = accessor.ByteStride(view);
    const usize elem_size = sizeof(float) * 4;
    const usize byte_start = as<usize>(view.byteOffset) + as<usize>(accessor.byteOffset);

    MLE_ASSERT_LOG(stride == 0 || stride >= elem_size, "Invalid stride for VEC4 accessor");

    out.resize(count);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    const auto* base = buffer.data.data() + byte_start;
    const usize step = (stride != 0) ? stride : elem_size;

    for (usize i = 0; i < count; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
        const auto* p = rAs<const float*>(base + (i * step));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
        out[i] = quat{p[3], p[0], p[1], p[2]};
    }

    return out;
}

std::vector<mat4f> GLTF::readAccessorMat4f(const tinygltf::Accessor& accessor) const {
    std::vector<mat4f> out;

    const auto& view = getBufferView(accessor);
    const auto& buffer = getBuffer(view);

    MLE_ASSERT_LOG(accessor.type == TINYGLTF_TYPE_MAT4, "Expected MAT4 accessor");
    MLE_ASSERT_LOG(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expected FLOAT component type for MAT4 accessor");

    const usize count = accessor.count;
    const usize stride = accessor.ByteStride(view);
    const usize elem_size = sizeof(float) * 16;
    const usize byte_start = as<usize>(view.byteOffset) + as<usize>(accessor.byteOffset);

    MLE_ASSERT_LOG(stride == 0 || stride >= elem_size, "Invalid stride for MAT4 accessor");

    out.resize(count);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    const auto* base = buffer.data.data() + byte_start;
    const usize step = (stride != 0) ? stride : elem_size;

    for (usize i = 0; i < count; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
        const auto* p = rAs<const float*>(base + (i * step));
        mat4f m{};
        for (int j = 0; j < 16; ++j) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
            glm::value_ptr(m)[j] = p[j];
        }
        out[i] = m;
    }

    return out;
}

std::vector<f32> GLTF::readAccessorFloat(const tinygltf::Accessor& accessor) const {
    std::vector<f32> out;

    const auto& view = getBufferView(accessor);
    const auto& buffer = getBuffer(view);

    MLE_ASSERT_LOG(accessor.type == TINYGLTF_TYPE_SCALAR, "Expected SCALAR accessor");
    MLE_ASSERT_LOG(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expected FLOAT component type for SCALAR accessor");

    const usize count = accessor.count;
    const usize stride = accessor.ByteStride(view);
    const usize elem_size = sizeof(float);
    const usize byte_start = as<usize>(view.byteOffset) + as<usize>(accessor.byteOffset);

    MLE_ASSERT_LOG(stride == 0 || stride >= elem_size, "Invalid stride for SCALAR accessor");

    out.resize(count);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    const auto* base = buffer.data.data() + byte_start;
    const usize step = (stride != 0) ? stride : elem_size;

    for (usize i = 0; i < count; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
        const auto* p = rAs<const float*>(base + (i * step));
        out[i] = *p;
    }

    return out;
}

std::vector<vec4u> GLTF::readAccessorJoints4u(const tinygltf::Accessor& accessor) const {
    std::vector<vec4u> out;

    const auto& view = getBufferView(accessor);
    const auto& buffer = getBuffer(view);

    MLE_ASSERT_LOG(accessor.type == TINYGLTF_TYPE_VEC4, "JOINTS_0 accessor must be VEC4");
    MLE_ASSERT_LOG(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE || accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
                       accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,
                   "Unsupported JOINTS_0 component type");

    const usize count = accessor.count;
    const usize stride = accessor.ByteStride(view);

    const usize elem_size = [&]() {
        switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                return sizeof(u8) * 4;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                return sizeof(u16) * 4;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                return sizeof(u32) * 4;
            default:
                MLE_UNREACHABLE;
        }
    }();

    const usize byte_start = as<usize>(view.byteOffset) + as<usize>(accessor.byteOffset);

    MLE_ASSERT_LOG(stride == 0 || stride >= elem_size, "Invalid stride for JOINTS_0 accessor");

    out.resize(count);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    const auto* base = buffer.data.data() + byte_start;
    const usize step = (stride != 0) ? stride : elem_size;

    switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            for (usize i = 0; i < count; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
                const auto* p = rAs<const u8*>(base + (i * step));
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
                out[i] = vec4u{as<u32>(p[0]), as<u32>(p[1]), as<u32>(p[2]), as<u32>(p[3])};
            }
        } break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            for (usize i = 0; i < count; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
                const auto* p = rAs<const u16*>(base + (i * step));
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
                out[i] = vec4u{as<u32>(p[0]), as<u32>(p[1]), as<u32>(p[2]), as<u32>(p[3])};
            }
        } break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
            for (usize i = 0; i < count; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
                const auto* p = rAs<const u32*>(base + (i * step));
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
                out[i] = vec4u{p[0], p[1], p[2], p[3]};
            }
        } break;
        default:
            MLE_UNREACHABLE;
    }

    return out;
}

std::vector<vec4f> GLTF::readAccessorWeights4f(const tinygltf::Accessor& accessor) const {
    std::vector<vec4f> out;

    const auto& view = getBufferView(accessor);
    const auto& buffer = getBuffer(view);

    MLE_ASSERT_LOG(accessor.type == TINYGLTF_TYPE_VEC4, "WEIGHTS_0 accessor must be VEC4");

    const usize count = accessor.count;
    const usize stride = accessor.ByteStride(view);

    const usize elem_size = [&]() {
        switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                return sizeof(float) * 4;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                return sizeof(u8) * 4;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                return sizeof(u16) * 4;
            default:
                return 0UL;
        }
    }();

    MLE_ASSERT_LOG(elem_size != 0, "Unsupported WEIGHTS_0 component type");

    const usize byte_start = as<usize>(view.byteOffset) + as<usize>(accessor.byteOffset);

    MLE_ASSERT_LOG(stride == 0 || stride >= elem_size, "Invalid stride for WEIGHTS_0 accessor");

    out.resize(count);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    const auto* base = buffer.data.data() + byte_start;
    const usize step = (stride != 0) ? stride : elem_size;

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
        for (usize i = 0; i < count; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
            const auto* p = rAs<const float*>(base + (i * step));
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
            out[i] = vec4f{p[0], p[1], p[2], p[3]};
        }
    } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        constexpr float INV_MAX = 1.0F / 255.0F;
        for (usize i = 0; i < count; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
            const auto* p = rAs<const u8*>(base + (i * step));
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
            out[i] = vec4f{as<f32>(p[0]) * INV_MAX, as<f32>(p[1]) * INV_MAX, as<f32>(p[2]) * INV_MAX, as<f32>(p[3]) * INV_MAX};
        }
    } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        constexpr float INV_MAX = 1.0F / 65535.0F;
        for (usize i = 0; i < count; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
            const auto* p = rAs<const u16*>(base + (i * step));
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
            out[i] = vec4f{as<f32>(p[0]) * INV_MAX, as<f32>(p[1]) * INV_MAX, as<f32>(p[2]) * INV_MAX, as<f32>(p[3]) * INV_MAX};
        }
    } else {
        MLE_UNREACHABLE;
    }

    return out;
}

std::vector<u32> GLTF::readIndicesU32(const tinygltf::Accessor& accessor, u32 baseVertex) const {
    std::vector<u32> out;

    const auto& view = getBufferView(accessor);
    const auto& buffer = getBuffer(view);

    MLE_ASSERT_LOG(accessor.type == TINYGLTF_TYPE_SCALAR, "Index accessor must be SCALAR");

    const usize count = accessor.count;
    const usize stride = accessor.ByteStride(view);
    const usize byte_start = as<usize>(view.byteOffset) + as<usize>(accessor.byteOffset);

    out.reserve(out.size() + count);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    const auto* base = buffer.data.data() + byte_start;

    auto read_loop = [&](auto dummy) {
        using CompT = decltype(dummy);
        const usize elem_size = sizeof(CompT);
        const usize step = (stride != 0) ? stride : elem_size;

        for (usize i = 0; i < count; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
            const auto* p = rAs<const CompT*>(base + (i * step));
            out.push_back(as<u32>(*p) + baseVertex);
        }
    };

    switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            read_loop(as<u8>(0));
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            read_loop(as<u16>(0));
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            read_loop(as<u32>(0));
            break;
        default:
            MLE_ASSERT_LOG(false, "Unsupported index component type");
            break;
    }

    return out;
}

}  // namespace mle
