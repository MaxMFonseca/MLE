#pragma once

#include "mle/client/Layer.h"
#include "mle/ui/UI.h"

namespace mle::user {
class UITestLayer : public mle::client::Layer {
  public:
    MLE_NO_COPY_MOVE(UITestLayer)

    UITestLayer() = default;
    ~UITestLayer() override = default;

    void init() override;
    void shutdown() override;

    void update() override;
    ImageRef render() override;

  private:
    UI ui_;
    mle::client::WindowSizedRenderTarget render_target_;
};
}  // namespace mle::user
