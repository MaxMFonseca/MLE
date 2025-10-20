#pragma once

#include "../Types.h"

namespace mle::ui::comp {
struct Name {
    std::string o;
};

struct Relationship {
    entt::entity parent = entt::null;
    entt::entity left = entt::null;
    entt::entity right = entt::null;
    entt::entity first_child = entt::null;
    usize child_count = 0;
};

struct ContainerNeedsInternalBoundsUpdateFlag {};
struct RequestExternalBoundsUpdateFlag {};

}  // namespace mle::ui::comp
