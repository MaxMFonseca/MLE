#pragma once

#include "../Element.h"

namespace mle::ui::element_renderable {
class ListContainer final : public Element::Container {
  public:
    enum class Layout : i8 { ROWS, COLS };

  public:
    ListContainer(ListContainer&&) = delete;
    ListContainer& operator=(ListContainer&&) = delete;
    ListContainer(const ListContainer&) = delete;
    ListContainer& operator=(const ListContainer&) = delete;

    explicit ListContainer(ElementRef element);
    ~ListContainer() override = default;

    void set(const sol::table& obj) override;

    void updateChildrenBounds() override;
    void updateChildrenBoundsDirty();
    void updateChildrenBoundsRows();
    void updateChildrenBoundsCols();

    static void registerLuaTypes();

  private:
    struct RemainginSizeHelper {
        f32 content = 0.0F;
        f32 padding_a = 0.0F;
        f32 padding_b = 0.0F;
        f32 margin_a = 0.0F;
        f32 margin_b = 0.0F;
    };
    void calcMainBound(const Element::Bound& target, f32& new_bound, f32& rem, f32& remaining_size, f32& shares, bool is_x, bool is_content = false) const;
    void calcOffBound(const Element::Bound& target, f32& new_bound, f32& rem, f32& shares, f32& remaining_size, bool is_x, bool is_content = false,
                      f32 child_ar = 0, f32 main_new_bound = 0) const;
    static void finishMainBoundRemaining(f32& new_bound, f32 rem, f32 share_size);
    void finishChildBoundsCalc(ElementRef child, Element::NewBounds& child_new_bounds);

  private:
    Layout layout_ = Layout::ROWS;
    f32 static_share_size_ = 0.0F;
    Element::Bound spacing_{};
};
}  // namespace mle::ui::element_renderable
