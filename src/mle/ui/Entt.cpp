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

[[nodiscard]] std::string Entt::name() const {
    const auto* parent_r = tryGet<comp::Parent>();
    if (!parent_r) {
        return "<root>";
    }
    const auto& parent_container = ui_.getRegistry().get<comp::Container>(parent_r->o);
    return parent_container.o.getNameFromE(e_);
};

[[nodiscard]] std::string Entt::parentName() const {
    const auto* parent_r = tryGet<comp::Parent>();
    if (!parent_r) {
        return "<no parent>";
    }
    Entt parent_entt{ui_, parent_r->o};
    return parent_entt.name();
}
}  // namespace mle::ui
