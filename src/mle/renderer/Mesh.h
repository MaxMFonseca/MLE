#pragma once

#include <tiny_gltf.h>

#include "mle/math/Types.h"
#include "mle/renderer/GLTF.h"
#include "mle/renderer/Types.h"

namespace mle {
class Mesh {
  public:
    struct Vertex {
        vec3f pos;
        vec3f normal;
        vec3f color;
        vec3f mre;
        int color_mix;
    };

    struct VertexSkin {
        vec3f pos;
        vec3f normal;
        vec3f color;
        vec3f mre;
        vec4u joint0;
        vec4f weight0;
        int color_mix;
    };

  public:
    void load(const GLTF& gltf, usize mesh_idx = 0);

    [[nodiscard]] BufferRef getVertexBuffer() const { return vertex_buffer_.get(); }
    [[nodiscard]] BufferRef getIndexBuffer() const { return index_buffer_.get(); }
    [[nodiscard]] usize getIndexCount() const { return index_count_; }
    [[nodiscard]] bool isSkinned() const { return skinned_; }

    [[nodiscard]] auto min() const { return min_; }
    [[nodiscard]] auto max() const { return max_; }

  private:
    BufferHnd vertex_buffer_;
    BufferHnd index_buffer_;
    usize index_count_ = 0;
    bool skinned_ = false;
    vec3f min_{+FLT_MAX, +FLT_MAX, +FLT_MAX};
    vec3f max_{-FLT_MAX, -FLT_MAX, -FLT_MAX};
};

}  // namespace mle
