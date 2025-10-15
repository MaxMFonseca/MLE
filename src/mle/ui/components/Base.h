#pragma once

#include "../Types.h"

namespace mle::ui::comp {
struct Parent {
    entt::entity o = entt::null;
};

struct RequestInternalUpdate {};
struct RequestExternalUpdate {};

}  // namespace mle::ui::comp
