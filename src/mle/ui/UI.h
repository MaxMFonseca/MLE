#pragma once

#include "mle/core/Result.h"
#include "mle/lua/Lua.h"
#include "mle/ui/systems/Bounds.h"
#include "mle/ui/systems/Events.h"
#include "mle/ui/systems/Hover.h"
#include "mle/ui/systems/LuaElementOps.h"
#include "mle/ui/systems/Rendering.h"
#include "mle/utils/ECS.h"
#include "mle/window/Events.h"
#include "mle/window/Types.h"
#include "sol/forward.hpp"

namespace mle {
class UI {
  public:
    UI();
    ~UI();

    void setRoot(const std::string& element_name);
    void setRoot(sol::table root_table);
    void clear();
    void shutdown() { clear(); }

    [[nodiscard]] auto& getRegistry() { return registry_; }
    [[nodiscard]] auto getRoot() const { return root_; }
    [[nodiscard]] vec2u getRootMaxSize() const { return root_max_size_; }
    [[nodiscard]] f32 getRootAspectRatio() const { return root_aspect_ratio_; }

    [[nodiscard]] Expected<std::reference_wrapper<const sol::table>> getStyle(std::string_view style_name);
    void addStyle(const std::string& style_name, const sol::table& style_table);

    void update();
    ImageRef render();
    [[nodiscard]] Expected<ImageRef> getRenderImage(entt::entity entity);

    Expected<ui::Entt> getE(std::span<const std::string_view> tree = {});
    Expected<ui::Entt> getE(const std::string& id);

    [[nodiscard]] entt::entity findNearestPopupRoot(entt::entity entity);
    [[nodiscard]] bool hasPopups() const;
    [[nodiscard]] u32 getNextPopupStackIndex(entt::entity parent_popup) const;
    void trimPopupStackTo(entt::entity popup_root);

    auto& eventSystem() { return events_; }
    auto& hoverSystem() { return hover_system_; }
    auto& boundsSystem() { return bounds_system_; }
    auto& renderingSystem() { return rendering_system_; }

  private:
    void addRootStyles(const sol::object& obj);
    void destroyFlagged();
    void handlePopupMousePress();
    [[nodiscard]] bool isPopupAncestorOf(entt::entity maybe_ancestor, entt::entity popup) const;

  private:
    entt::registry registry_;
    entt::entity root_ = entt::null;
    vec2u root_max_size_{0};
    f32 root_aspect_ratio_ = 1;

    std::map<std::string, sol::table> styles_;

    ui::system::Bounds bounds_system_{*this};
    ui::system::Rendering rendering_system_{*this};
    ui::system::Hover hover_system_{*this};
    ui::system::Events events_{*this};

    window::ev::ResizeL window_resize_el_;  // FIXME: This should not be here, the user should do this explicitly
    KeyListenerHnd popup_mouse_listener_;
};
}  // namespace mle
