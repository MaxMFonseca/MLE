#pragma once

#include "mle/common/Utils.h"
#include "mle/core/App.h"

namespace mle_cubes {
class App : public mle::core::App {
  public:
    MLE_NO_COPY_MOVE(App)

    App() = default;
    ~App() override = default;

    void init() override;
    void shutdown() override;

    void update() override;

  private:
};
}  // namespace mle_cubes
