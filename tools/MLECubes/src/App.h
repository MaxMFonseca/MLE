#pragma once

#include "Editor.h"
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
    std::unique_ptr<Editor> editor_;
};
}  // namespace mle_cubes
