#pragma once

#include "mle/math/Types.h"
#include "mle/ui/Entt.h"

namespace mle::ui::comp {
class FreeContainer {
  public:
    static void apply(const Entt& e, const sol::object& obj);

    [[nodiscard]] vec2u calculateChildrenBounds(const Entt& e, vec2u max_size) const;

    void set(const sol::table& table);

    void setScrollable(bool scrollable) { scrollable_ = scrollable; }
    [[nodiscard]] bool isScrollable() const { return scrollable_; }
    void setOffset(vec2i offset) { offset_ = offset; }
    [[nodiscard]] vec2i getOffset() const { return offset_; }
    void setOffsetX(i32 x) { offset_.x = x; }
    [[nodiscard]] i32 getOffsetX() const { return offset_.x; }
    void setOffsetY(i32 y) { offset_.y = y; }
    [[nodiscard]] i32 getOffsetY() const { return offset_.y; }

    // NOLINTBEGIN(readability-identifier-naming, readability-avoid-const-params-in-decls) Not my declaration
    static void on_construct(entt::registry& registry, const entt::entity entt);
    static void on_destroy(entt::registry& registry, const entt::entity entt);
    // NOLINTEND(readability-identifier-naming, readability-avoid-const-params-in-decls)

  private:
    bool scrollable_ = true;
    bool pack_children_ = false;
    vec2i offset_ = {0, 0};
};
}  // namespace mle::ui::comp
