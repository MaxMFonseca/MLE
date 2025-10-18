#pragma once

#include "../Types.h"

namespace mle::ui::comp {
struct Parent {
    entt::entity o = entt::null;
};

struct ContainerNeedsInternalBoundsUpdateFlag {};
struct RequestExternalBoundsUpdateFlag {};

}  // namespace mle::ui::comp
