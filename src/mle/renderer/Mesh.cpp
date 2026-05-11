#include "Mesh.h"

#include "mle/math/Types3D.h"
#include "mle/renderer/Buffer.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Types.h"
#include "mle/utils/ID.h"

namespace mle {
// NOLINTNEXTLINE(readability-function-cognitive-complexity) ok
void Mesh::load(const GLTF& gltf, usize mesh_idx) {
    const auto& model = gltf.model();

    std::vector<Vertex> vertices;
    std::vector<VertexSkin> vertices_skinned;
    std::vector<u32> indices;

    index_count_ = 0;

    MLE_ASSERT_LOG(mesh_idx < model.meshes.size(), "Mesh index out of range");
    const auto& mesh = model.meshes.at(mesh_idx);

    color_multiplier_name_.clear();
    if (mesh.extras.IsObject() && mesh.extras.Has("color_multiplier")) {
        const auto& value = mesh.extras.Get("color_multiplier");
        MLE_ASSERT_LOG(value.IsString(), "mesh extras.color_multiplier must be a string");
        color_multiplier_name_ = value.Get<std::string>();
    }

    MLE_ASSERT_LOG(mesh.primitives.size() == 1, "Only single primitive meshes supported");

    const auto& prim = mesh.primitives.front();
    if (prim.mode != TINYGLTF_MODE_TRIANGLES) {
        MLE_E("Unsupported primitive mode {}, only TRIANGLES supported. Skipping primitive.", prim.mode);
        return;
    }

    const auto pos_it = prim.attributes.find("POSITION");
    MLE_ASSERT_LOG(pos_it != prim.attributes.end(), "Primitive without POSITION attribute");

    const auto& pos_acc = gltf.getAccessor(pos_it->second);

    std::vector<vec3f> positions = gltf.readAccessorVec3f(pos_acc);
    const usize vert_count = positions.size();

    min_.x = as<f32>(pos_acc.minValues[0]);
    min_.y = as<f32>(pos_acc.minValues[1]);
    min_.z = as<f32>(pos_acc.minValues[2]);
    max_.x = as<f32>(pos_acc.maxValues[0]);
    max_.y = as<f32>(pos_acc.maxValues[1]);
    max_.z = as<f32>(pos_acc.maxValues[2]);

    std::vector<vec3f> normals;
    if (auto it = prim.attributes.find("NORMAL"); it != prim.attributes.end()) {
        const auto& acc = gltf.getAccessor(it->second);
        normals = gltf.readAccessorVec3f(acc);
        MLE_ASSERT_LOG(normals.size() == vert_count, "NORMAL count does not match POSITION count");
    } else {
        normals.resize(vert_count, vec3f{0.0F, 1.0F, 0.0F});
    }

    std::vector<vec3f> colors;
    if (auto it = prim.attributes.find("COLOR_0"); it != prim.attributes.end()) {
        const auto& acc = gltf.getAccessor(it->second);
        colors = gltf.readAccessorVec3f(acc);
        MLE_ASSERT_LOG(colors.size() == vert_count, "COLOR_0 count does not match POSITION count");
    } else {
        colors.resize(vert_count, vec3f{1.0F, 1.0F, 1.0F});
    }

    std::vector<float> metals;
    std::vector<float> roughs;
    std::vector<float> emiss;

    if (auto it = prim.attributes.find("_M"); it != prim.attributes.end()) {
        const auto& acc = gltf.getAccessor(it->second);
        metals = gltf.readAccessorFloat(acc);
        MLE_ASSERT_LOG(metals.size() == vert_count, "_M count does not match POSITION count");
    } else {
        metals.resize(vert_count, 0.0F);
    }
    if (auto it = prim.attributes.find("_R"); it != prim.attributes.end()) {
        const auto& acc = gltf.getAccessor(it->second);
        roughs = gltf.readAccessorFloat(acc);
        MLE_ASSERT_LOG(roughs.size() == vert_count, "_R count does not match POSITION count");
    } else {
        roughs.resize(vert_count, 0.5F);
    }

    if (auto it = prim.attributes.find("_E"); it != prim.attributes.end()) {
        const auto& acc = gltf.getAccessor(it->second);
        emiss = gltf.readAccessorFloat(acc);
        MLE_ASSERT_LOG(emiss.size() == vert_count, "_E count does not match POSITION count");
    } else {
        emiss.resize(vert_count, 0.0F);
    }

    std::vector<vec4u> joints0;
    std::vector<vec4f> weights0;

    if (auto it = prim.attributes.find("JOINTS_0"); it != prim.attributes.end()) {
        skinned_ = true;
        const auto& acc = gltf.getAccessor(it->second);
        joints0 = gltf.readAccessorJoints4u(acc);
        MLE_ASSERT_LOG(joints0.size() == vert_count, "JOINTS_0 count does not match POSITION count");
    }

    if (auto it = prim.attributes.find("WEIGHTS_0"); it != prim.attributes.end()) {
        skinned_ = true;
        const auto& acc = gltf.getAccessor(it->second);
        weights0 = gltf.readAccessorWeights4f(acc);
        MLE_ASSERT_LOG(weights0.size() == vert_count, "WEIGHTS_0 count does not match POSITION count");
    }

    if (skinned_) {
        MLE_ASSERT_LOG(!joints0.empty(), "Skinned primitive without JOINTS_0");
        MLE_ASSERT_LOG(!weights0.empty(), "Skinned primitive without WEIGHTS_0");
    }

    const u32 base_vertex = skinned_ ? as<u32>(vertices_skinned.size()) : as<u32>(vertices.size());
    if (skinned_) {
        vertices_skinned.reserve(vertices_skinned.size() + vert_count);
    } else {
        vertices.reserve(vertices.size() + vert_count);
    }

    if (skinned_) {
        for (usize i = 0; i < vert_count; ++i) {
            VertexSkin v{};
            v.pos = positions[i];
            v.normal = normals[i];
            v.color = colors[i];
            v.mre = vec3f{metals[i], roughs[i], emiss[i]};
            v.joint0 = joints0[i];
            v.weight0 = weights0[i];
            vertices_skinned.push_back(v);
        }
    } else {
        for (usize i = 0; i < vert_count; ++i) {
            Vertex v{};
            v.pos = positions[i];
            v.normal = normals[i];
            v.color = colors[i];
            v.mre = vec3f{metals[i], roughs[i], emiss[i]};
            vertices.push_back(v);
        }
    }

    if (prim.indices >= 0) {
        const auto& idx_acc = gltf.getAccessor(prim.indices);
        indices = gltf.readIndicesU32(idx_acc, base_vertex);
    } else {
        indices.reserve(indices.size() + vert_count);
        for (usize i = 0; i < vert_count; ++i) {
            indices.push_back(base_vertex + i);
        }
    }

    index_count_ = indices.size();

    auto& cmd_mgr = Renderer::i().commandManager();
    auto ots_cmd = cmd_mgr.getOTS(GCmdType::T);

    // TODO: make this async

    BufferHnd vsb;
    if (skinned_) {
        Buffer::CI vb_ci;
        vb_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY;
        vb_ci.size = sizeof(VertexSkin) * vertices_skinned.size();
        vb_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        vertex_buffer_ = Buffer::createHnd(vb_ci);

        vsb = vertex_buffer_->writeStaged(ots_cmd, vertices_skinned.data(), vb_ci.size);
        vertex_buffer_->ownershipRelease(ots_cmd, cmd_mgr.queueDataIdx(GCmdType::G));
    } else {
        Buffer::CI vb_ci;
        vb_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY;
        vb_ci.size = sizeof(Vertex) * vertices.size();
        vb_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        vertex_buffer_ = Buffer::createHnd(vb_ci);

        vsb = vertex_buffer_->writeStaged(ots_cmd, vertices.data(), vb_ci.size);
        vertex_buffer_->ownershipRelease(ots_cmd, cmd_mgr.queueDataIdx(GCmdType::G));
    }

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
