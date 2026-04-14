#pragma once

#include <tiny_gltf.h>

#include <string>
#include <vector>

#include "mle/math/Types.h"
#include "mle/renderer/GLTF.h"

namespace mle {

class Skeleton {
  public:
    struct Joint {
        int parent = -1;
        int node_index = -1;
        mat4f inverse_bind;
        std::string name;
    };

  public:
    void loadFromGLTF(const GLTF& gltf);

    [[nodiscard]] const std::vector<Joint>& getJoints() const { return joints_; }
    [[nodiscard]] auto jointCount() const { return joints_.size(); }

    void buildSkinMatrices(const std::vector<mat4f>& node_globals, std::vector<mat4f>& out_skin_mats) const;

  private:
    std::vector<Joint> joints_;
};
}  // namespace mle
