#include "Mesh.h"

#include <glm/gtx/norm.hpp>

#include "mle/math/Types3D.h"
#include "mle/renderer/Buffer.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/TextureCache.h"
#include "mle/renderer/Types.h"
#include "mle/utils/ID.h"

namespace mle {
namespace {
bool hasTexture(const tinygltf::Material& material) {
    const auto& pbr = material.pbrMetallicRoughness;
    return pbr.baseColorTexture.index >= 0 || pbr.metallicRoughnessTexture.index >= 0 || material.normalTexture.index >= 0 ||
           material.occlusionTexture.index >= 0 || material.emissiveTexture.index >= 0;
}

vec4f readVec4(const std::vector<double>& values, const vec4f& fallback) {
    if (values.size() < 4) {
        return fallback;
    }
    return vec4f{as<f32>(values[0]), as<f32>(values[1]), as<f32>(values[2]), as<f32>(values[3])};
}

vec3f readVec3(const std::vector<double>& values, const vec3f& fallback) {
    if (values.size() < 3) {
        return fallback;
    }
    return vec3f{as<f32>(values[0]), as<f32>(values[1]), as<f32>(values[2])};
}

ImageRef loadTextureImage(const GLTF& gltf, int texture_index, bool srgb, std::string_view usage_name) {
    if (texture_index < 0) {
        return nullptr;
    }

    const auto& model = gltf.model();
    MLE_ASSERT_LOG(texture_index < as<int>(model.textures.size()), "Texture index out of range");

    const auto& texture = model.textures[as<usize>(texture_index)];
    if (texture.source < 0) {
        MLE_W("GLTF texture '{}' has no source image", usage_name);
        return nullptr;
    }

    const auto image_index = as<usize>(texture.source);
    const auto& image = model.images.at(image_index);
    const std::string texture_name = !texture.name.empty() ? texture.name : image.name;
    const std::string cache_key =
        fmt::format("gltf:{}:{}:{}:{}:{}", gltf.sourcePath().generic_string(), image_index, texture_name, usage_name, srgb ? "srgb" : "linear");
    const entt::id_type texture_id = entt::hashed_string::value(cache_key.c_str());

    auto& texture_cache = Renderer::i().textureCache();
    if (texture_cache.contains(texture_id)) {
        if (auto cached = texture_cache.get(texture_id); cached.has_value()) {
            return cached.value();
        }
    }

    auto raw = gltf.readImageRawData(image_index, srgb);
    MLE_D("Uploading GLB texture '{}' ({}x{}, {}), usage '{}'", image.name, raw.extent.x, raw.extent.y, srgb ? "sRGB" : "linear", usage_name);
    return texture_cache.addTextureWait(texture_id, raw);
}

Mesh::PbrMaterial loadMaterial(const GLTF& gltf, int material_index) {
    auto& texture_cache = Renderer::i().textureCache();

    Mesh::PbrMaterial out{};
    out.base_color_texture = texture_cache.getWhiteTexture();
    out.metallic_roughness_texture = texture_cache.getWhiteTexture();
    out.normal_texture = texture_cache.getFlatNormalTexture();
    out.occlusion_texture = texture_cache.getWhiteTexture();
    out.emissive_texture = texture_cache.getBlackTexture();

    const auto& model = gltf.model();
    if (material_index < 0) {
        return out;
    }

    MLE_ASSERT_LOG(material_index < as<int>(model.materials.size()), "Material index out of range");
    const auto& material = model.materials[as<usize>(material_index)];
    const auto& pbr = material.pbrMetallicRoughness;

    out.base_color_factor = readVec4(pbr.baseColorFactor, out.base_color_factor);
    out.emissive_factor = readVec3(material.emissiveFactor, out.emissive_factor);
    out.metallic_factor = as<f32>(pbr.metallicFactor);
    out.roughness_factor = as<f32>(pbr.roughnessFactor);
    out.normal_scale = as<f32>(material.normalTexture.scale);
    out.occlusion_strength = as<f32>(material.occlusionTexture.strength);
    out.double_sided = material.doubleSided;

    if (auto* image = loadTextureImage(gltf, pbr.baseColorTexture.index, true, "baseColor")) {
        out.base_color_texture = image;
    }
    if (auto* image = loadTextureImage(gltf, pbr.metallicRoughnessTexture.index, false, "metallicRoughness")) {
        out.metallic_roughness_texture = image;
    }
    if (auto* image = loadTextureImage(gltf, material.normalTexture.index, false, "normal")) {
        out.normal_texture = image;
    }
    if (auto* image = loadTextureImage(gltf, material.occlusionTexture.index, false, "occlusion")) {
        out.occlusion_texture = image;
    }
    if (auto* image = loadTextureImage(gltf, material.emissiveTexture.index, true, "emissive")) {
        out.emissive_texture = image;
    }

    return out;
}

std::vector<vec4f> generateTangents(const std::vector<vec3f>& positions, const std::vector<vec3f>& normals, const std::vector<vec2f>& uvs,
                                    const std::vector<u32>& indices) {
    std::vector<vec3f> tan1(positions.size(), vec3f{0.0F});
    std::vector<vec3f> tan2(positions.size(), vec3f{0.0F});
    std::vector<vec4f> tangents(positions.size(), vec4f{1.0F, 0.0F, 0.0F, 1.0F});

    for (usize i = 0; i + 2 < indices.size(); i += 3) {
        const u32 i0 = indices[i + 0];
        const u32 i1 = indices[i + 1];
        const u32 i2 = indices[i + 2];

        const vec3f e1 = positions[i1] - positions[i0];
        const vec3f e2 = positions[i2] - positions[i0];
        const vec2f d1 = uvs[i1] - uvs[i0];
        const vec2f d2 = uvs[i2] - uvs[i0];

        const f32 denom = d1.x * d2.y - d2.x * d1.y;
        if (std::abs(denom) <= 1.0e-8F) {
            continue;
        }

        const f32 inv = 1.0F / denom;
        const vec3f tangent = (e1 * d2.y - e2 * d1.y) * inv;
        const vec3f bitangent = (e2 * d1.x - e1 * d2.x) * inv;

        tan1[i0] += tangent;
        tan1[i1] += tangent;
        tan1[i2] += tangent;
        tan2[i0] += bitangent;
        tan2[i1] += bitangent;
        tan2[i2] += bitangent;
    }

    for (usize i = 0; i < positions.size(); ++i) {
        const vec3f n = glm::normalize(normals[i]);
        vec3f t = tan1[i] - n * glm::dot(n, tan1[i]);
        if (glm::length2(t) <= 1.0e-8F) {
            const vec3f up = std::abs(n.y) < 0.999F ? vec3f{0.0F, 1.0F, 0.0F} : vec3f{1.0F, 0.0F, 0.0F};
            t = glm::cross(up, n);
        }
        t = glm::normalize(t);
        const f32 handedness = glm::dot(glm::cross(n, t), tan2[i]) < 0.0F ? -1.0F : 1.0F;
        tangents[i] = vec4f{t, handedness};
    }

    return tangents;
}

template <typename VertexT>
BufferHnd uploadVertices(const std::vector<VertexT>& vertices) {
    auto& cmd_mgr = Renderer::i().commandManager();
    auto ots_cmd = cmd_mgr.getOTS(GCmdType::T);

    Buffer::CI vb_ci;
    vb_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY;
    vb_ci.size = sizeof(VertexT) * vertices.size();
    vb_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    auto vertex_buffer = Buffer::createHnd(vb_ci);

    auto vsb = vertex_buffer->writeStaged(ots_cmd, vertices.data(), vb_ci.size);
    vertex_buffer->ownershipRelease(ots_cmd, cmd_mgr.queueDataIdx(GCmdType::G));

    cmd_mgr.submitOTSWait(std::move(ots_cmd));

    auto g_ots_cmd = cmd_mgr.getOTS(GCmdType::G);
    vertex_buffer->ownershipAcquire(g_ots_cmd);
    cmd_mgr.submitOTSWait(std::move(g_ots_cmd));

    return vertex_buffer;
}
}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity) GLB primitive ingestion is necessarily broad.
void Mesh::load(const GLTF& gltf, usize mesh_idx) {
    const auto& model = gltf.model();

    std::vector<PbrColorVertex> color_vertices;
    std::vector<PbrColorSkinnedVertex> color_vertices_skinned;
    std::vector<PbrTextureVertex> texture_vertices;
    std::vector<PbrTextureSkinnedVertex> texture_vertices_skinned;
    std::vector<u32> indices;

    index_count_ = 0;
    skinned_ = false;
    textured_ = false;
    vertex_kind_ = VertexKind::PBR_COLOR;

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

    material_ = loadMaterial(gltf, prim.material);
    if (prim.material >= 0) {
        textured_ = hasTexture(model.materials.at(as<usize>(prim.material)));
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

    std::vector<vec2f> uvs;
    if (auto it = prim.attributes.find("TEXCOORD_0"); it != prim.attributes.end()) {
        const auto& acc = gltf.getAccessor(it->second);
        uvs = gltf.readAccessorVec2f(acc);
        MLE_ASSERT_LOG(uvs.size() == vert_count, "TEXCOORD_0 count does not match POSITION count");
    } else if (textured_) {
        MLE_W("Textured primitive without TEXCOORD_0; falling back to PBR color vertex path");
        textured_ = false;
    }

    std::vector<vec3f> colors;
    if (auto it = prim.attributes.find("COLOR_0"); it != prim.attributes.end()) {
        const auto& acc = gltf.getAccessor(it->second);
        colors = gltf.readAccessorColor3f(acc);
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
        metals.resize(vert_count, 1.0F);
    }
    if (auto it = prim.attributes.find("_R"); it != prim.attributes.end()) {
        const auto& acc = gltf.getAccessor(it->second);
        roughs = gltf.readAccessorFloat(acc);
        MLE_ASSERT_LOG(roughs.size() == vert_count, "_R count does not match POSITION count");
    } else {
        roughs.resize(vert_count, 1.0F);
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

    if (prim.indices >= 0) {
        const auto& idx_acc = gltf.getAccessor(prim.indices);
        indices = gltf.readIndicesU32(idx_acc, 0);
    } else {
        indices.reserve(vert_count);
        for (usize i = 0; i < vert_count; ++i) {
            indices.push_back(as<u32>(i));
        }
    }

    if (textured_) {
        std::vector<vec4f> tangents;
        if (auto it = prim.attributes.find("TANGENT"); it != prim.attributes.end()) {
            const auto& acc = gltf.getAccessor(it->second);
            tangents = gltf.readAccessorVec4f(acc);
            MLE_ASSERT_LOG(tangents.size() == vert_count, "TANGENT count does not match POSITION count");
        } else {
            tangents = generateTangents(positions, normals, uvs, indices);
        }

        if (skinned_) {
            vertex_kind_ = VertexKind::PBR_TEXTURE_SKINNED;
            texture_vertices_skinned.reserve(vert_count);
            for (usize i = 0; i < vert_count; ++i) {
                texture_vertices_skinned.push_back(PbrTextureSkinnedVertex{.pos = positions[i],
                                                                           .normal = normals[i],
                                                                           .uv = uvs[i],
                                                                           .tangent = tangents[i],
                                                                           .joint0 = joints0[i],
                                                                           .weight0 = weights0[i]});
            }
            vertex_buffer_ = uploadVertices(texture_vertices_skinned);
        } else {
            vertex_kind_ = VertexKind::PBR_TEXTURE;
            texture_vertices.reserve(vert_count);
            for (usize i = 0; i < vert_count; ++i) {
                texture_vertices.push_back(PbrTextureVertex{.pos = positions[i], .normal = normals[i], .uv = uvs[i], .tangent = tangents[i]});
            }
            vertex_buffer_ = uploadVertices(texture_vertices);
        }
    } else if (skinned_) {
        vertex_kind_ = VertexKind::PBR_COLOR_SKINNED;
        color_vertices_skinned.reserve(vert_count);
        for (usize i = 0; i < vert_count; ++i) {
            color_vertices_skinned.push_back(PbrColorSkinnedVertex{.pos = positions[i],
                                                                   .normal = normals[i],
                                                                   .color = colors[i],
                                                                   .mre = vec3f{metals[i], roughs[i], emiss[i]},
                                                                   .joint0 = joints0[i],
                                                                   .weight0 = weights0[i]});
        }
        vertex_buffer_ = uploadVertices(color_vertices_skinned);
    } else {
        vertex_kind_ = VertexKind::PBR_COLOR;
        color_vertices.reserve(vert_count);
        for (usize i = 0; i < vert_count; ++i) {
            color_vertices.push_back(PbrColorVertex{.pos = positions[i], .normal = normals[i], .color = colors[i], .mre = vec3f{metals[i], roughs[i], emiss[i]}});
        }
        vertex_buffer_ = uploadVertices(color_vertices);
    }

    index_count_ = indices.size();

    auto& cmd_mgr = Renderer::i().commandManager();
    auto ots_cmd = cmd_mgr.getOTS(GCmdType::T);

    Buffer::CI ib_ci;
    ib_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY;
    ib_ci.size = sizeof(u32) * indices.size();
    ib_ci.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    index_buffer_ = Buffer::createHnd(ib_ci);

    auto isb = index_buffer_->writeStaged(ots_cmd, indices.data(), ib_ci.size);
    index_buffer_->ownershipRelease(ots_cmd, cmd_mgr.queueDataIdx(GCmdType::G));

    cmd_mgr.submitOTSWait(std::move(ots_cmd));

    auto g_ots_cmd = cmd_mgr.getOTS(GCmdType::G);
    index_buffer_->ownershipAcquire(g_ots_cmd);
    cmd_mgr.submitOTSWait(std::move(g_ots_cmd));
}

}  // namespace mle
