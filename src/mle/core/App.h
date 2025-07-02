#pragma once

#include "mle/common/Utils.h"

namespace mle::core {
class App {
  public:
    MLE_NO_COPY_MOVE(App)

    App() = default;
    virtual ~App() = default;

    virtual void init() = 0;
    virtual void shutdown() = 0;

    virtual void update() = 0;
    virtual void render() = 0;

  private:
};
};  // namespace mle::core
