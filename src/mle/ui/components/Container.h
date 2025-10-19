#pragma once

#include <utility>

#include "../Types.h"
#include "mle/ui/components/Bounds.h"
#include "mle/ui/components/Utils.h"
#include "mle/utils/Justify.h"
#include "mle/utils/Utils.h"

// TODO: make this a class AND FIX THE NAMING
namespace mle::ui::comp {
struct Container {
    enum class Type : u8 { HYBRID, LIST, FREE };
    enum class ListDirection : u8 { HORIZONTAL, VERTICAL, HORIZONTAL_REVERSED, VERTICAL_REVERSED };
    enum class WrapMode : u8 { NO, WRAP, WRAP_REVERSED };
    enum class AlignCross : u8 { START, CENTER, END, STRETCH };
    using JustifyMode = JustifyInt::LineMode;

    EntityStorage o;

    int offset_x = 0, offset_y = 0;

    bool scrollable = true;

    Type type = Type::HYBRID;
    ListDirection list_direction = ListDirection::VERTICAL;
    JustifyMode list_align_main = JustifyMode::START;
    AlignCross list_align_cross = AlignCross::START;
    WrapMode wrap_mode = WrapMode::NO;
    TargetBound max_size_cross;
    TargetBound min_gap_main;
    TargetBound min_gap_cross;
    TargetBound max_fit_size_main;
    TargetBound max_fit_size_cross;

    sol::table element_base;

    Container() = default;
    ~Container() = default;
    MLE_NO_COPY_MOVE(Container)

    void set(const Entt& e, const sol::table& table);
    void addChild(const Entt& e, const sol::table& table, const std::string& name = "", usize pos = max<usize>());
    void addMany(const Entt& e, const sol::table& table);
    void setOffset(vec2i offset) {
        offset_x = offset.x;
        offset_y = offset.y;
    }
    void setOffsetX(int offset_x_) { offset_x = offset_x_; }
    void setOffsetY(int offset_y_) { offset_y = offset_y_; }
    void setScrollable(bool scrollable_) { scrollable = scrollable_; }
    void setListDirection(ListDirection direction) { list_direction = direction; }
    void setListAlignMain(JustifyMode align) { list_align_main = align; }
    void setListAlignCross(AlignCross align) { list_align_cross = align; }
    void setWrapMode(WrapMode mode) { wrap_mode = mode; }
    void setMaxSizeCross(TargetBound max_size) { max_size_cross = max_size; }
    void setMinGapMain(TargetBound min_gap) { min_gap_main = min_gap; }
    void setMinGapCross(TargetBound min_gap) { min_gap_cross = min_gap; }
    void setMaxFitSizeMain(TargetBound max_fit_size) { max_fit_size_main = max_fit_size; }
    void setMaxFitSizeCross(TargetBound max_fit_size) { max_fit_size_cross = max_fit_size; }
    void setElementBase(sol::table base) { element_base = std::move(base); }

    [[nodiscard]] vec2u provideSize(const Entt& e, vec2u max_size) const { return calculateChildrenBounds(e, max_size); }
    [[nodiscard]] vec2u calculateChildrenBounds(const Entt& e, vec2u max_size) const;

    static void apply(const Entt& e, const sol::object& obj);
    static void applyAddChild(const Entt& e, const sol::object& obj);
    static void applyAddChildren(const Entt& e, const sol::object& obj);

    // NOLINTBEGIN(readability-identifier-naming, readability-avoid-const-params-in-decls) Not my declaration
    static void on_construct(entt::registry& registry, const entt::entity entt);
    static void on_destroy(entt::registry& registry, const entt::entity entt);
    // NOLINTEND(readability-identifier-naming, readability-avoid-const-params-in-decls)

  private:
    std::tuple<sol::table, entt::entity, std::string, usize> createChildEnttHnd(const Entt& e, const sol::table& table, std::string name, usize pos);
    void addChild(const Entt& e, entt::entity child_e, const sol::table& comp_table);

  private:
};
}  // namespace mle::ui::comp
