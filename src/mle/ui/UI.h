#pragma once

#include "mle/lua/Lua.h"
#include "mle/ui/systems/Bounds.h"
#include "mle/ui/systems/Hover.h"
#include "mle/ui/systems/LuaElementOps.h"
#include "mle/ui/systems/Rendering.h"
#include "mle/utils/ECS.h"
#include "mle/window/Events.h"
#include "sol/forward.hpp"

namespace mle {
class UI {
  public:
    UI();

    void setRoot(const std::string& element_name);
    void setRoot(sol::table root_table);
    void resizeRoot(const vec2u& size);
    void clear();
    void shutdown() { clear(); }

    [[nodiscard]] auto& getRegistry() { return registry_; }
    [[nodiscard]] auto getRoot() const { return root_; }
    [[nodiscard]] auto& getLua() { return lua_; }
    [[nodiscard]] auto& getLuaElementOps() { return lua_element_ops_; }
    [[nodiscard]] sol::table getTableFor(const std::string& element_name);
    [[nodiscard]] vec2u getRootSize() const { return root_size_; }
    [[nodiscard]] f32 getRootAspectRatio() const { return root_aspect_ratio_; }

    [[nodiscard]] Expected<std::reference_wrapper<const sol::table>> getStyle(std::string_view style_name);
    void addStyle(const std::string& style_name, const sol::table& style_table);

    void update();
    ImageRef render();

    Expected<ui::Entt> getE(std::span<const std::string_view> tree = {});

  private:
    void addRootStyles(const sol::object& obj);

  private:
    entt::registry registry_;
    entt::entity root_ = entt::null;
    vec2u root_size_{0};
    f32 root_aspect_ratio_ = 1;
    Lua lua_;

    std::map<std::string, sol::table> styles_;

    ui::system::Bounds bounds_system_{*this};
    ui::system::LuaElementOps lua_element_ops_{*this};
    ui::system::Rendering rendering_system_{*this};
    ui::system::Hover hover_system_{*this};

    window::ev::ResizeL window_resize_el_;  // FIXME: This should not be here, the user should do this explicitly
};
}  // namespace mle
