#pragma once

#include "Container.h"

namespace mle::ui::element {
struct ListLayout : Layout {
    static void lkhList(entt::entity self, const sol::object& obj);

    Axis axis = Axis::Y;

    void updateChildrenBounds(entt::entity self, Recti context, bool force_update) const override;
    void updateChildrenBoundsY(entt::entity self, Recti context, bool force_update) const;
    void updateChildrenBoundsX(entt::entity self, Recti context, bool force_update) const;
};
}  // namespace mle::ui::element
