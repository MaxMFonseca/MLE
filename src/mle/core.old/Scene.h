#pragma once

#include "mle/common/Utils.h"
#include "mle/common/math/Types.h"

namespace mle::core {
class Scene {
  public:
    MLE_NO_COPY_MOVE(Scene)

    Scene() = default;
    virtual ~Scene() = default;

    virtual void init() {};
    virtual void shutdown() {};

    virtual void update() {};
    virtual void render([[maybe_unused]] f64 dt) {};

  private:
};
}  // namespace mle::core
