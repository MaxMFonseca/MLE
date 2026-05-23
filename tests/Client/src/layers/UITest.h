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

    ImageRef getImage();

  private:
    UI ui_;

    std::array<ImageHnd, 2> images_;
};
}  // namespace mle::user
