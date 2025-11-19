#pragma once

#include <tiny_gltf.h>

#include "mle/math/Types.h"
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

  public:
    void load(const tinygltf::Model& model);

    [[nodiscard]] BufferRef getVertexBuffer() const { return vertex_buffer_.get(); }
    [[nodiscard]] BufferRef getIndexBuffer() const { return index_buffer_.get(); }
    [[nodiscard]] usize getIndexCount() const { return index_count_; }

  private:
    BufferHnd vertex_buffer_;
    BufferHnd index_buffer_;
    usize index_count_ = 0;
};
}  // namespace mle
