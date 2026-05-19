#pragma once

#include <tiny_gltf.h>

#include <string>

#include "mle/math/Types.h"
#include "mle/renderer/GLTF.h"
#include "mle/renderer/Types.h"

namespace mle {
class Mesh {
  public:
    enum class VertexKind : u8 {
        PBR_COLOR,
        PBR_COLOR_SKINNED,
        PBR_TEXTURE,
        PBR_TEXTURE_SKINNED,
    };

    struct PbrMaterial {
        vec4f base_color_factor{1.0F, 1.0F, 1.0F, 1.0F};
        vec3f emissive_factor{0.0F, 0.0F, 0.0F};
        f32 metallic_factor = 1.0F;
        f32 roughness_factor = 1.0F;
        f32 normal_scale = 1.0F;
        f32 occlusion_strength = 1.0F;
        bool double_sided = false;
        ImageRef base_color_texture = nullptr;
        ImageRef metallic_roughness_texture = nullptr;
        ImageRef normal_texture = nullptr;
        ImageRef occlusion_texture = nullptr;
        ImageRef emissive_texture = nullptr;
    };

    struct PbrColorVertex {
        vec3f pos;
        vec3f normal;
        vec3f color;
        vec3f mre;
    };

    struct PbrColorSkinnedVertex {
        vec3f pos;
        vec3f normal;
        vec3f color;
        vec3f mre;
        vec4u joint0;
        vec4f weight0;
    };

    struct PbrTextureVertex {
        vec3f pos;
        vec3f normal;
        vec2f uv;
        vec4f tangent;
    };

    struct PbrTextureSkinnedVertex {
        vec3f pos;
        vec3f normal;
        vec2f uv;
        vec4f tangent;
        vec4u joint0;
        vec4f weight0;
    };

  public:
    void load(const GLTF& gltf, usize mesh_idx = 0, usize primitive_idx = 0);

    [[nodiscard]] BufferRef getVertexBuffer() const { return vertex_buffer_.get(); }
    [[nodiscard]] BufferRef getIndexBuffer() const { return index_buffer_.get(); }
    [[nodiscard]] usize getIndexCount() const { return index_count_; }
    [[nodiscard]] bool isSkinned() const { return skinned_; }
    [[nodiscard]] bool isTextured() const { return textured_; }
    [[nodiscard]] VertexKind getVertexKind() const { return vertex_kind_; }
    [[nodiscard]] const PbrMaterial& getMaterial() const { return material_; }
    [[nodiscard]] const std::string& getColorMultiplierName() const { return color_multiplier_name_; }

    [[nodiscard]] auto min() const { return min_; }
    [[nodiscard]] auto max() const { return max_; }

  private:
    BufferHnd vertex_buffer_;
    BufferHnd index_buffer_;
    usize index_count_ = 0;
    bool skinned_ = false;
    bool textured_ = false;
    VertexKind vertex_kind_ = VertexKind::PBR_COLOR;
    PbrMaterial material_{};
    vec3f min_{+FLT_MAX, +FLT_MAX, +FLT_MAX};
    vec3f max_{-FLT_MAX, -FLT_MAX, -FLT_MAX};
    std::string color_multiplier_name_;
};

}  // namespace mle
