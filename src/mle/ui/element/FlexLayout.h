#pragma once

#include "Container.h"

namespace mle ::ui::element {
struct FlexLayout : Layout {
    static void lkhFlex(entt::entity self, const sol::object& obj);

    void updateChildrenBounds(entt::entity self, bool force_update) const override;
};
}  // namespace mle::ui::element
