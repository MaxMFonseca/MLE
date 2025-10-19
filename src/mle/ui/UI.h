#pragma once

#include "mle/lua/Lua.h"
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
    [[nodiscard]] vec2u getRootSize() const { return root_size_; }
    [[nodiscard]] f32 getRootAspectRatio() const { return root_aspect_ratio_; }

    void update();

  private:
    entt::registry registry_;
    entt::entity root_{};
    vec2u root_size_{0};
    f32 root_aspect_ratio_ = 1;
    Lua lua_;

    ui::system::Bounds bounds_system_{*this};
    ui::system::LuaElementOps lua_element_ops_{*this};
};
}  // namespace mle
