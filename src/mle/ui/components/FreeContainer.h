#pragma once

#include "mle/math/Types.h"
#include "mle/ui/Entt.h"

namespace mle::ui::comp {
class FreeContainer {
  public:
    static void apply(const Entt& e, const sol::object& obj);
    static void applyAddScrollY(const Entt& e, const sol::object& obj);
    static void applyScrollSensitivity(const Entt& e, const sol::object& obj);

    [[nodiscard]] vec2u calculateChildrenBounds(const Entt& e, vec2u max_size) const;

    void set(const sol::table& table);

    void setScrollable(bool scrollable) { scrollable_y_ = scrollable; }
    [[nodiscard]] bool isScrollable() const { return scrollable_y_; }
    void setMaxScrollY(int max_scroll_y) { max_scroll_y_ = max_scroll_y; }
    [[nodiscard]] int getMaxScrollY() const { return max_scroll_y_; }
    void setCurrentScrollY(int val) { scroll_y_ = std::clamp(val, 0, max_scroll_y_); }
    [[nodiscard]] int getCurrentScrollY() const { return scroll_y_; }
    void setScrollSensitivity(int scroll_sensitivity) { scroll_sensitivity_ = scroll_sensitivity; }
    [[nodiscard]] int getScrollSensitivity() const { return scroll_sensitivity_; }

    // NOLINTBEGIN(readability-identifier-naming, readability-avoid-const-params-in-decls) Not my declaration
    static void on_construct(entt::registry& registry, const entt::entity entt);
    static void on_destroy(entt::registry& registry, const entt::entity entt);
    // NOLINTEND(readability-identifier-naming, readability-avoid-const-params-in-decls)

  private:
    bool pack_children_ = false;

    bool scrollable_y_ = true;
    int max_scroll_y_ = 0;
    int scroll_y_ = 0;
    int scroll_sensitivity_ = 10;
};
}  // namespace mle::ui::comp
