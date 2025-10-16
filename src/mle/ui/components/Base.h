#pragma once

#include "../Types.h"

namespace mle::ui::comp {
struct Parent {
    entt::entity o = entt::null;
};

struct ContainerNeedsInternalBoundsUpdate {};
struct RequestExternalBoundsUpdate {};

}  // namespace mle::ui::comp
