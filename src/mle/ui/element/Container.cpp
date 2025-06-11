#include "Container.h"

#include "mle/lua/Utils.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"

namespace mle::ui::element::comp {
Container::Container(LayoutHnd&& layout) :
    layout_(std::move(layout)) {
}

void Container::addChildren(entt::entity self, const sol::table& table) {
    for (const auto& [key, val] : table) {
        if (key.is<std::string>()) {
            continue;
        }

        addChild(self, val, lua::getKeyOr(val, "pos", max<usize>()));
    }
}

void Container::addChild(entt::entity self, const sol::table& table, usize pos) {
    auto& reg = getRegistry();
    auto child_entt = reg.create();
    applyTable(child_entt, table, self);

    children_.add(child_entt, pos);
}

void Container::updateChildrenBounds(entt::entity self, Recti context, bool verify_all, bool force_update) const {  // NOLINT
    MLE_ASSERT(layout_);
    auto& bounds = getRegistry().get<comp::Bounds>(self).bounds;

    if (!isFitX(self)) {
        context.pos.x = bounds.top();
        context.size.x = bounds.width();
    }
    if (!isFitY(self)) {
        context.pos.y = bounds.left();
        context.size.y = bounds.height();
    }

    if (!changed_bounds_children_.empty()) {
        layout_->updateChildrenBounds(self, context, force_update);
    }
    if (verify_all) {
        for (auto c : children_.get()) {
            auto* child_container = getRegistry().try_get<Container>(c);
            if (child_container) {
                child_container->updateChildrenBounds(c, context, verify_all, force_update);
            }
        }
    }
}

void Container::notifyChildChangedBounds(entt::entity self, entt::entity child) {  // NOLINT
    auto& reg = getRegistry();
    auto& self_container = reg.get<Container>(self);
    self_container.changed_bounds_children_.emplace(child);

    if (isFit(self)) {
        notifyChildChangedBounds(reg.get<comp::Parent>(self).parent, self);
    } else {
        reg.emplace<ChildChangedBounds>(self);  // Only mark non fit containers with this so tree traversal may not be needed
    }
}

}  // namespace mle::ui::element::comp
