#pragma once

#include <array>

#include "mle/renderer/Types.h"
#include "mle/utils/Color.h"
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
    virtual ImageRef render() { return nullptr; };
    virtual void renderTo([[maybe_unused]] ImageRef target) {};

  private:
};

class WindowSizedRenderTarget {
  public:
    MLE_NO_COPY_MOVE(WindowSizedRenderTarget)

    WindowSizedRenderTarget() = default;
    ~WindowSizedRenderTarget();

    ImageRef getImage(Color clear_color = Color::ZERO);

  private:
    std::array<ImageHnd, 2> images_;
};
}  // namespace mle::client
