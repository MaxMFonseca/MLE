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
        std::string name;
    };

  public:
    void loadFromGLTF(const GLTF& gltf);

    [[nodiscard]] const std::vector<Joint>& getJoints() const { return joints_; }
    [[nodiscard]] auto jointCount() const { return joints_.size(); }

  private:
    std::vector<Joint> joints_;
};
}  // namespace mle
