#pragma once

#include "mle/utils/Utils.h"

namespace mle {
class Renderer {
    MLE_SINGLETON(Renderer)

  public:
    void init();
    void shutdown();

  private:
};
}  // namespace mle
