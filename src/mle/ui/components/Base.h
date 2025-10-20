#pragma once

#include "../Types.h"
#include "mle/utils/Color.h"

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

struct Background {
    Color color = Color::ZERO;

    void set(const sol::object& obj) { color = Color::fromLua(obj); }

    static void apply(const Entt& e, const sol::object& obj);
};

struct ContainerNeedsInternalBoundsUpdateFlag {};
struct RequestExternalBoundsUpdateFlag {};

}  // namespace mle::ui::comp
