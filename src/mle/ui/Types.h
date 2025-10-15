#pragma once

#include "mle/lua/Types.h"
#include "mle/math/Types.h"
#include "mle/utils/ECS.h"

namespace mle {
class UI;

namespace ui {
struct Entt {
    UI& ui;
    entt::entity e;

    Entt(UI& ui, entt::entity entity) :
        ui(ui),
        e(entity) {}
};
}  // namespace ui
}  // namespace mle
