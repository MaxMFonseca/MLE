#include "Entt.h"

#include "UI.h"
#include "mle/ui/components/Bounds.h"

namespace mle::ui {
bool Entt::hasFitSize() const {
    if (const auto* size = tryGet<comp::TargetSize>()) {
        if (size->x.type == TargetBound::Type::FIT || size->y.type == TargetBound::Type::FIT) {
            return true;
        }
        if ((size->x.type == TargetBound::Type::DEFAULT || size->y.type == TargetBound::Type::DEFAULT) && has<comp::SizeProvider>()) {
            return true;
        }
    }
    return false;
}
}  // namespace mle::ui
