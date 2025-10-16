#pragma once

#include "mle/lua/Lua.h"
#include "mle/lua/Types.h"
#include "mle/renderer/Types.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Container.h"
#include "mle/ui/systems/Bounds.h"
#include "mle/ui/systems/LuaElementOps.h"
#include "mle/utils/ECS.h"

namespace mle {
class UI {
  public:
    UI() = default;

    void setRoot(const std::string& element_name);
    void resizeRoot(const vec2u& size);

    [[nodiscard]] auto& getRegistry() { return registry_; }
    [[nodiscard]] auto getRoot() const { return root_; }
    [[nodiscard]] auto& getLua() { return lua_; }
    [[nodiscard]] auto& getLuaElementOps() { return lua_element_ops_; }
    [[nodiscard]] sol::table getTableFor(const std::string& element_name);

  private:
    entt::registry registry_;
    entt::entity root_{};
    Lua lua_;

    ui::system::Bounds bounds_system_{*this};
    ui::system::LuaElementOps lua_element_ops_{*this};
};
}  // namespace mle
