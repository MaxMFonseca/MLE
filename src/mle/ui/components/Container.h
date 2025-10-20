#pragma once

#include <utility>

#include "../Types.h"
#include "mle/ui/components/Bounds.h"
#include "mle/utils/Justify.h"

namespace mle::ui::comp {
class Container {
  public:
    enum class Type : u8 { HYBRID, LIST, FREE };
    enum class ListDirection : u8 { HORIZONTAL, VERTICAL, HORIZONTAL_REVERSED, VERTICAL_REVERSED };
    enum class ListWrapMode : u8 { NO, WRAP, WRAP_REVERSED };
    enum class ListCrossAlign : u8 { START, CENTER, END, STRETCH };
    using ListJustify = JustifyInt::LineMode;

    Container() = default;

    [[nodiscard]] static std::vector<entt::entity> getChildren(const Entt& e);

    [[nodiscard]] vec2u calculateChildrenBounds(const Entt& e, vec2u max_size) const;

    void set(const Entt& e, const sol::table& table);

    void setType(Type type) { type_ = type; }
    [[nodiscard]] Type getType() const { return type_; }
    void setScrollable(bool scrollable) { scrollable_ = scrollable; }
    [[nodiscard]] bool isScrollable() const { return scrollable_; }
    void setOffset(vec2i offset) { offset_ = offset; }
    [[nodiscard]] vec2i getOffset() const { return offset_; }
    void setOffsetX(i32 x) { offset_.x = x; }
    [[nodiscard]] i32 getOffsetX() const { return offset_.x; }
    void setOffsetY(i32 y) { offset_.y = y; }
    [[nodiscard]] i32 getOffsetY() const { return offset_.y; }
    void setChildrenBase(sol::table table) { children_base_ = std::move(table); }
    [[nodiscard]] sol::table getChildrenBase() const { return children_base_; }
    void setListDirection(ListDirection direction) { list_direction_ = direction; }
    [[nodiscard]] ListDirection getListDirection() const { return list_direction_; }
    void setListJustify(ListJustify justify) { list_justify_ = justify; }
    [[nodiscard]] ListJustify getListJustify() const { return list_justify_; }
    void setListCrossAlign(ListCrossAlign align) { list_cross_align_ = align; }
    [[nodiscard]] ListCrossAlign getListCrossAlign() const { return list_cross_align_; }
    void setListWrapMode(ListWrapMode mode) { list_wrap_mode_ = mode; }
    [[nodiscard]] ListWrapMode getListWrapMode() const { return list_wrap_mode_; }
    void setListCrossMaxSize(TargetBound tb) { list_cross_max_size_ = tb; }
    [[nodiscard]] TargetBound getListCrossMaxSize() const { return list_cross_max_size_; }
    void setListMainMinGap(TargetBound tb) { list_main_min_gap_ = tb; }
    [[nodiscard]] TargetBound getListMainMinGap() const { return list_main_min_gap_; }
    void setListCrossGap(TargetBound tb) { list_cross_gap_ = tb; }
    [[nodiscard]] TargetBound getListCrossGap() const { return list_cross_gap_; }
    void setListMainMaxFitSize(TargetBound tb) { list_main_max_fit_size_ = tb; }
    [[nodiscard]] TargetBound getListMainMaxFitSize() const { return list_main_max_fit_size_; }
    void setListCrossMaxFitSize(TargetBound tb) { list_cross_max_fit_size_ = tb; }
    [[nodiscard]] TargetBound getListCrossMaxFitSize() const { return list_cross_max_fit_size_; }

    void setType(std::string_view type) { setType(strToType(type)); }
    void setListDirection(std::string_view direction) { setListDirection(strToListDirection(direction)); }
    void setListJustify(std::string_view justify) { setListJustify(strToListJustify(justify)); }
    void setListCrossAlign(std::string_view align) { setListCrossAlign(strToListCrossAlign(align)); }
    void setListWrapMode(std::string_view mode) { setListWrapMode(strToListWrapMode(mode)); }

    static void apply(const Entt& e, const sol::object& obj);
    static void applyAddChild(const Entt& e, const sol::object& obj);
    static void applyAddChildren(const Entt& e, const sol::object& obj);

    // NOLINTBEGIN(readability-identifier-naming, readability-avoid-const-params-in-decls) Not my declaration
    static void on_construct(entt::registry& registry, const entt::entity entt);
    static void on_destroy(entt::registry& registry, const entt::entity entt);
    // NOLINTEND(readability-identifier-naming, readability-avoid-const-params-in-decls)

    static Type strToType(std::string_view str);
    static ListDirection strToListDirection(std::string_view str);
    static ListJustify strToListJustify(std::string_view str);
    static ListCrossAlign strToListCrossAlign(std::string_view str);
    static ListWrapMode strToListWrapMode(std::string_view str);

    static void destroyChild(const Entt& e, entt::entity child_e);
    static void destroyAllChildren(const Entt& e);

  private:
    [[nodiscard]] static entt::entity createChildHnd(const Entt& e, usize idx = max<usize>());
    [[nodiscard]] static entt::entity getChildAt(const Entt& e, usize idx);
    [[nodiscard]] static Expected<usize> getChildIdx(const Entt& e, std::string_view name);
    [[nodiscard]] static entt::entity getChildByName(const Entt& e, std::string_view name);
    [[nodiscard]] static usize getChildrenCount(const Entt& e);

    void createChildren(const Entt& e, const sol::table& table);
    void createChild(const Entt& e, const sol::table& table);
    void applyChildInitialTable(const Entt& e, const sol::table& table, entt::entity child_e);

  private:
    Type type_ = Type::HYBRID;
    bool scrollable_ = true;
    vec2i offset_ = {0, 0};
    sol::table children_base_;

    ListDirection list_direction_ = ListDirection::VERTICAL;
    ListJustify list_justify_ = ListJustify::START;
    ListCrossAlign list_cross_align_ = ListCrossAlign::START;
    ListWrapMode list_wrap_mode_ = ListWrapMode::NO;
    TargetBound list_cross_max_size_;
    TargetBound list_main_min_gap_;
    TargetBound list_cross_gap_;
    TargetBound list_main_max_fit_size_;
    TargetBound list_cross_max_fit_size_;
};
}  // namespace mle::ui::comp
