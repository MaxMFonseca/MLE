#pragma once

#include "mle/lua/Lua.h"
#include "mle/lua/Types.h"
#include "mle/renderer/Types.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Container.h"
#include "mle/ui/systems/LuaElementOps.h"
#include "mle/utils/ECS.h"

namespace mle {
class UI {
  public:
    UI() = default;

    void setRoot(sol::table root_table);

    [[nodiscard]] auto& getRegistry() { return registry_; }
    [[nodiscard]] auto getRoot() const { return root_; }
    [[nodiscard]] auto& getLua() { return lua_; }
    [[nodiscard]] auto& getLuaElementOps() { return lua_element_ops_; }

    const ui::comp::Parent& getParent(entt::entity e) const;
    const ui::comp::Container& getParentContainer(entt::entity e) const;

  private:
    entt::registry registry_;
    entt::entity root_{};
    Lua lua_;
    ui::system::LuaElementOps lua_element_ops_{*this};
};
}  // namespace mle
