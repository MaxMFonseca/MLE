#pragma once

#include <array>

#include "SimulationClock.h"
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

    virtual void update([[maybe_unused]] const UpdateTiming& timing) {};
    virtual ImageRef render([[maybe_unused]] const RenderTiming& timing) { return nullptr; };
    virtual void renderTo([[maybe_unused]] ImageRef target, [[maybe_unused]] const RenderTiming& timing) {};

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
