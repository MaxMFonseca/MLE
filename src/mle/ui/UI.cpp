#include "UI.h"

namespace mle {
const ui::comp::Parent& UI::getParent(entt::entity e) const {
    MLE_ASSERT_LOG(registry_.all_of<ui::comp::Parent>(e), "Trying to get root's parent?");
    return registry_.get<ui::comp::Parent>(e);
}

const ui::comp::Container& UI::getParentContainer(entt::entity e) const {
    return registry_.get<ui::comp::Container>(getParent(e).o);
}
}  // namespace mle
