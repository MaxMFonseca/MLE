#pragma once

#include <tiny_gltf.h>

#include "mle/math/Types.h"
#include "mle/renderer/GLTF.h"
#include "mle/renderer/Types.h"

namespace mle {
class Model {
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
    void load(const GLTF& gltf);

    [[nodiscard]] BufferRef getVertexBuffer() const { return vertex_buffer_.get(); }
    [[nodiscard]] BufferRef getIndexBuffer() const { return index_buffer_.get(); }
    [[nodiscard]] usize getIndexCount() const { return index_count_; }

  private:
    BufferHnd vertex_buffer_;
    BufferHnd index_buffer_;
    usize index_count_ = 0;
    bool skinned_ = false;
};
}  // namespace mle
