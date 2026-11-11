#pragma once

#include "mle/client/Layer.h"
#include "mle/ui/UI.h"

namespace mle::user {
class InitLayer : public mle::client::Layer {
  public:
    MLE_NO_COPY_MOVE(InitLayer)

    InitLayer() = default;
    ~InitLayer() override = default;

    void init() override;
    void shutdown() override;

    void update(const mle::client::UpdateTiming& timing) override;
    ImageRef render(const mle::client::RenderTiming& timing) override;

  private:
    UI ui_;
    mle::client::WindowSizedRenderTarget render_target_;
};
}  // namespace mle::user
