#pragma once

#include <entt/entt.hpp>

#include "mle/lua/Types.h"

namespace mle::ui::element {  // NOLINT
struct EntityWrapper {
    EntityWrapper(entt::entity o) :  // NOLINT
        o(o) {};
    entt::entity o;

    void apply(const std::string& key, const sol::object& obj) const;
    [[nodiscard]] sol::table getTable() const;
    [[nodiscard]] bool isHovered() const;
};
using EWrap = EntityWrapper;

namespace comp {
class Container;
}  // namespace comp
}  // namespace mle::ui::element
