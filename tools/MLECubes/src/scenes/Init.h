#pragma once

#include "mle/common/Utils.h"
#include "mle/core/Scene.h"

namespace mle_cubes {
using namespace mle;
class Init : public core::Scene {
  public:
    Init() = default;
    ~Init() = default;

    void init() override;
    void update() override;

  private:
};
}  // namespace mle_cubes
