#pragma once

#include "Types.h"
#include "mle/renderer/Types.h"

namespace mle::ui {
void init();
void shutdown();
void update();
renderer::ImageRef render();
entt::registry& getRegistry();

}  // namespace mle::ui
