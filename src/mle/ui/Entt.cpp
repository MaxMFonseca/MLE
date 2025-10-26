#include "Entt.h"

#include "UI.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Bounds.h"
#include "mle/utils/ECS.h"

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
        if (((size->x.type == TargetBound::Type::DEFAULT && size->x.val == 0) || (size->y.type == TargetBound::Type::DEFAULT && size->y.val == 0)) &&
            has<comp::SizeProvider>()) {
            return true;
        }
    } else {
        return true;
    }
    return false;
}

std::string Entt::name() const {
    if (const auto* name_comp = tryGet<comp::Name>(); name_comp) {
        return name_comp->o;
    }
    return getRelationship().getParent() == entt::null ? "<root>" : "<unnamed>";
};

// NOLINTNEXTLINE(misc-no-recursion) No problem
std::string Entt::fullName() const {
    auto parent = getParent();
    if (parent == entt::null) {
        return "<root>";
    }
    return Entt{ui_, parent}.fullName() + "." + name();
}

void Entt::destroy() const {
    auto& relationship = getRelationship();
    if (relationship.getParent() != entt::null) {
        relationship.destroyChild(Entt{ui_, relationship.getParent()}, e_);
    } else {
        ui_.clear();
    }
}

Expected<Entt> Entt::getChild(std::span<const std::string_view> tree) const {
    MLE_ASSERT(!tree.empty());

    entt::entity cur = e();
    for (const std::string_view name : tree) {
        Entt ew = derive(cur);

        auto& relationship = ew.getRelationship();
        auto next_e = relationship.getChildByName(ew, name);
        if (next_e == entt::null) {
            return std::unexpected(Result::NOT_FOUND);
        }
        cur = next_e;
    }
    return derive(cur);
}
}  // namespace mle::ui
