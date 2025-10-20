#include "Entt.h"

#include "UI.h"
#include "mle/ui/components/Bounds.h"

namespace mle::ui {
void Entt::setName(const std::string& name) const {
    if (name.empty()) {
        erase<comp::Name>();
        return;
    }

    if (name.contains('.')) {
        MLE_W("Invalid character '.' in element name '{}'", name);
        return;
    }
    patchOrEmplace<comp::Name>([&](auto& n) { n.o = name; });
}

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

std::string Entt::name() const {
    const auto* name_comp = tryGet<comp::Name>();
    return name_comp ? name_comp->o : "<unnamed>";
};

std::string Entt::parentName() const {
    auto parent_r = getParent();
    if (parent_r == entt::null) {
        return "<no parent>";
    }
    Entt parent_entt{ui_, parent_r};
    return parent_entt.name();
}

// NOLINTNEXTLINE(misc-no-recursion) cool recursion
std::string Entt::fullName() const {
    auto parent_r = getParent();
    if (parent_r != entt::null) {
        Entt parent_entt{ui_, parent_r};
        return parent_entt.fullName() + "." + name();
    }
    return name();
}

void Entt::destroy() const {
    auto& relationship = getRelationship();
    if (relationship.parent != entt::null) {
        comp::Container::destroyChild(Entt{ui_, relationship.parent}, e_);
    } else {
        ui_.clear();
    }
}
}  // namespace mle::ui
