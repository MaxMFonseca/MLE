#pragma once

#include "mle/utils/Utils.h"

namespace mle::client {
class Layer {
  public:
    MLE_NO_COPY_MOVE(Layer)

    Layer() = default;
    virtual ~Layer() = default;

    virtual void init() {};
    virtual void shutdown() {};

    virtual void update() {};
    virtual void render([[maybe_unused]] f64 dt) {};

  private:
};
}  // namespace mle::client
