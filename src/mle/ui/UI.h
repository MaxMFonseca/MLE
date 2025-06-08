#pragma once

#include "mle/renderer/Types.h"
#include "mle/ui/detail/ElementManager.h"

namespace mle::ui {
void init();
void shutdown();
renderer::ImageRef render();

namespace detail {
ElementManager& getElementManager();
}  // namespace detail
}  // namespace mle::ui
