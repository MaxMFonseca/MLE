#include "Entt.h"

#include "UI.h"
#include "mle/ui/components/Bounds.h"

namespace mle::ui {
[[nodiscard]] bool Entt::anyFitTargetExternalBound() const {
    if (const auto* c = tryGet<comp::TargetPosition>(); c) {
        if (c->x.isFit() || c->y.isFit()) {
            return true;
        }
    }
    if (const auto* c = tryGet<comp::TargetSize>(); c) {
        if (c->x.isFit() || c->y.isFit()) {
            return true;
        }
    }
    if (const auto* c = tryGet<comp::TargetMargin>(); c) {
        if (c->t.isFit() || c->b.isFit() || c->l.isFit() || c->r.isFit()) {
            return true;
        }
    }
    return false;
}

}  // namespace mle::ui
