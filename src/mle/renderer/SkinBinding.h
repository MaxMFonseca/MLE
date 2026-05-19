#pragma once

#include <span>
#include <string>
#include <vector>

#include "mle/math/Types.h"
#include "mle/renderer/GLTF.h"

namespace mle {
class SkinBinding {
  public:
    struct JointBinding {
        int node_index = -1;
        mat4f inverse_bind{1.0F};
        std::string name;
    };

  public:
    void loadFromGLTF(const GLTF& gltf, usize skin_index = 0);

    [[nodiscard]] const std::string& getName() const { return name_; }
    [[nodiscard]] const std::vector<JointBinding>& getJoints() const { return joints_; }
    [[nodiscard]] auto jointCount() const { return joints_.size(); }

    void buildSkinMatrices(std::span<const mat4f> node_globals, std::span<mat4f> out_skin_mats) const;

  private:
    std::string name_;
    std::vector<JointBinding> joints_;
};
}  // namespace mle
