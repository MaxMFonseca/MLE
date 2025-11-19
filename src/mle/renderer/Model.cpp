#include "Model.h"

#include "mle/renderer/Buffer.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Types.h"

namespace mle {
namespace {
const tinygltf::Accessor& getAccessor(const tinygltf::Model& m, int idx) {
    MLE_ASSERT_LOG(idx >= 0 && idx < as<int>(m.accessors.size()), "Accessor index out of range");
    return m.accessors[as<usize>(idx)];
}

const tinygltf::BufferView& getBufferView(const tinygltf::Model& m, const tinygltf::Accessor& acc) {
    MLE_ASSERT_LOG(acc.bufferView >= 0 && acc.bufferView < as<int>(m.bufferViews.size()), "Accessor without valid bufferView");
    return m.bufferViews[as<usize>(acc.bufferView)];
}

const tinygltf::Buffer& getBuffer(const tinygltf::Model& m, const tinygltf::BufferView& view) {
    MLE_ASSERT_LOG(view.buffer >= 0 && view.buffer < as<int>(m.buffers.size()), "BufferView without valid buffer");
    return m.buffers[as<usize>(view.buffer)];
}

void readAccessorVec3f(const tinygltf::Model& model, const tinygltf::Accessor& accessor, std::vector<vec3f>& out) {
    const auto& view = getBufferView(model, accessor);
    const auto& buffer = getBuffer(model, view);

    MLE_ASSERT_LOG(accessor.type == TINYGLTF_TYPE_VEC3, "Expected VEC3({}) accessor, got type {}", TINYGLTF_TYPE_VEC3, accessor.type);
    MLE_ASSERT_LOG(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Expected FLOAT component type for VEC3");

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
}

void readAccessorFloat(const tinygltf::Model& model, const tinygltf::Accessor& accessor, std::vector<float>& out) {
    const auto& view = getBufferView(model, accessor);
    const auto& buffer = getBuffer(model, view);

    MLE_ASSERT_LOG(accessor.type == TINYGLTF_TYPE_SCALAR, "Expected SCALAR({}) accessor, got type {}", TINYGLTF_TYPE_SCALAR, accessor.type);
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
}

/// Reads accessor as indices into a u32 array.
inline void readIndicesU32(const tinygltf::Model& model, const tinygltf::Accessor& accessor, std::vector<u32>& out, u32 baseVertex) {
    const auto& view = getBufferView(model, accessor);
    const auto& buffer = getBuffer(model, view);

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
}
}  // namespace

void Model::load(const tinygltf::Model& model) {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;

    index_count_ = 0;

    for (const auto& mesh : model.meshes) {
        for (const auto& prim : mesh.primitives) {
            if (prim.mode != TINYGLTF_MODE_TRIANGLES) {
                MLE_E("Unsupported primitive mode {}, only TRIANGLES supported. Skipping primitive.", prim.mode);
                continue;
            }

            const auto pos_it = prim.attributes.find("POSITION");
            MLE_ASSERT_LOG(pos_it != prim.attributes.end(), "Primitive without POSITION attribute");

            const auto& pos_acc = getAccessor(model, pos_it->second);

            std::vector<vec3f> positions;
            readAccessorVec3f(model, pos_acc, positions);
            const usize vert_count = positions.size();

            std::vector<vec3f> normals(vert_count, vec3f{0.0F, 1.0F, 0.0F});
            if (auto it = prim.attributes.find("NORMAL"); it != prim.attributes.end()) {
                const auto& acc = getAccessor(model, it->second);
                readAccessorVec3f(model, acc, normals);
                MLE_ASSERT_LOG(normals.size() == vert_count, "NORMAL count does not match POSITION count");
            }

            std::vector<vec3f> colors(vert_count, vec3f{1.0F, 1.0F, 1.0F});
            if (auto it = prim.attributes.find("COLOR_0"); it != prim.attributes.end()) {
                const auto& acc = getAccessor(model, it->second);
                readAccessorVec3f(model, acc, colors);
                MLE_ASSERT_LOG(colors.size() == vert_count, "COLOR_0 count does not match POSITION count");
            }

            std::vector<float> metals(vert_count, 0.0F);
            std::vector<float> roughs(vert_count, 0.5F);
            std::vector<float> emiss(vert_count, 0.0F);

            if (auto it = prim.attributes.find("_M"); it != prim.attributes.end()) {
                const auto& acc = getAccessor(model, it->second);
                readAccessorFloat(model, acc, metals);
                MLE_ASSERT_LOG(metals.size() == vert_count, "_M count does not match POSITION count");
            }
            if (auto it = prim.attributes.find("_R"); it != prim.attributes.end()) {
                const auto& acc = getAccessor(model, it->second);
                readAccessorFloat(model, acc, roughs);
                MLE_ASSERT_LOG(roughs.size() == vert_count, "_R count does not match POSITION count");
            }
            if (auto it = prim.attributes.find("_E"); it != prim.attributes.end()) {
                const auto& acc = getAccessor(model, it->second);
                readAccessorFloat(model, acc, emiss);
                MLE_ASSERT_LOG(emiss.size() == vert_count, "_E count does not match POSITION count");
            }

            std::vector<int> color_mix(vert_count, -1);
            if (auto it = prim.attributes.find("_C"); it != prim.attributes.end()) {
                const auto& acc = getAccessor(model, it->second);
                std::vector<float> tmp;
                readAccessorFloat(model, acc, tmp);
                MLE_ASSERT_LOG(tmp.size() == vert_count, "_C count does not match POSITION count");
                for (usize i = 0; i < vert_count; ++i) {
                    color_mix[i] = as<int>(std::round(tmp[i]));
                }
            }

            const u32 base_vertex = as<u32>(vertices.size());
            vertices.reserve(vertices.size() + vert_count);

            for (usize i = 0; i < vert_count; ++i) {
                Vertex v{};
                v.pos = positions[i];
                v.normal = normals[i];
                v.color = colors[i];
                v.mre = vec3f{metals[i], roughs[i], emiss[i]};
                v.color_mix = color_mix[i];
                vertices.push_back(v);
            }

            if (prim.indices >= 0) {
                const auto& idx_acc = getAccessor(model, prim.indices);
                readIndicesU32(model, idx_acc, indices, base_vertex);
            } else {
                indices.reserve(indices.size() + vert_count);
                for (usize i = 0; i < vert_count; ++i) {
                    indices.push_back(base_vertex + i);
                }
            }
        }
    }

    index_count_ = indices.size();

    auto& cmd_mgr = Renderer::i().commandManager();
    auto ots_cmd = cmd_mgr.getOTS(GCmdType::T);

    // TODO: make this async

    Buffer::CI vb_ci;
    vb_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY;
    vb_ci.size = sizeof(Vertex) * vertices.size();
    vb_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    vertex_buffer_ = Buffer::createHnd(vb_ci);

    auto vsb = vertex_buffer_->writeStaged(ots_cmd, vertices.data(), vb_ci.size);
    vertex_buffer_->ownershipRelease(ots_cmd, cmd_mgr.queueDataIdx(GCmdType::G));

    Buffer::CI ib_ci;
    ib_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY;
    ib_ci.size = sizeof(u32) * indices.size();
    ib_ci.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    index_buffer_ = Buffer::createHnd(ib_ci);

    auto isb = index_buffer_->writeStaged(ots_cmd, indices.data(), ib_ci.size);
    index_buffer_->ownershipRelease(ots_cmd, cmd_mgr.queueDataIdx(GCmdType::G));

    cmd_mgr.submitOTSWait(std::move(ots_cmd));

    auto g_ots_cmd = cmd_mgr.getOTS(GCmdType::G);
    vertex_buffer_->ownershipAcquire(g_ots_cmd);
    index_buffer_->ownershipAcquire(g_ots_cmd);

    cmd_mgr.submitOTSWait(std::move(g_ots_cmd));
}
}  // namespace mle
