#pragma once

#include <functional>

#include "Types.h"
#include "mle/renderer/Types.h"
#include "mle/ui/element/Base.h"

namespace mle::ui {
void init();
void shutdown();
void update();
renderer::ImageRef render();
entt::registry& getRegistry();
}  // namespace mle::ui
