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

    void update() override;
    ImageRef render([[maybe_unused]] f64 dt) override;

  private:
    UI ui_;
};
}  // namespace mle::user
