#pragma once

#include "mle/math/Types.h"
#include "mle/ui/Entt.h"
#include "mle/utils/Justify.h"

namespace mle::ui::comp {

class ListContainer {
  public:
    enum class Direction : u8 { HORIZONTAL, VERTICAL, HORIZONTAL_REVERSED, VERTICAL_REVERSED };
    enum class WrapMode : u8 { NO, WRAP, WRAP_REVERSED };
    enum class CrossAlign : u8 { START, CENTER, END, STRETCH };
    using Justify = JustifyInt::LineMode;

    ListContainer() = default;

    [[nodiscard]] vec2u calculateChildrenBounds(const Entt& e, vec2u max_size) const;

    void set(const sol::table& table);

    void setScrollable(bool scrollable) { scrollable_ = scrollable; }
    [[nodiscard]] bool isScrollable() const { return scrollable_; }
    void setListDirection(Direction direction) { list_direction_ = direction; }
    [[nodiscard]] Direction getListDirection() const { return list_direction_; }
    void setListJustify(Justify justify) { list_justify_ = justify; }
    [[nodiscard]] Justify getListJustify() const { return list_justify_; }
    void setListJustifyLast(Justify justify) { list_justify_last_ = justify; }
    [[nodiscard]] Justify getListJustifyLast() const { return list_justify_last_; }
    void setListCrossAlign(CrossAlign align) { list_cross_align_ = align; }
    [[nodiscard]] CrossAlign getListCrossAlign() const { return list_cross_align_; }
    void setListWrapMode(WrapMode mode) { list_wrap_mode_ = mode; }
    [[nodiscard]] WrapMode getListWrapMode() const { return list_wrap_mode_; }
    void setListCrossMaxSize(TargetBound tb) { list_cross_max_size_ = tb; }
    [[nodiscard]] TargetBound getListCrossMaxSize() const { return list_cross_max_size_; }
    void setListMainMinGap(TargetBound tb) { list_main_min_gap_ = tb; }
    [[nodiscard]] TargetBound getListMainMinGap() const { return list_main_min_gap_; }
    void setListCrossGap(TargetBound tb) { list_cross_gap_ = tb; }
    [[nodiscard]] TargetBound getListCrossGap() const { return list_cross_gap_; }
    void setPackChildren(bool pack) { pack_children_ = pack; }
    [[nodiscard]] bool getPackChildren() const { return pack_children_; }

    [[nodiscard]] bool getListDirectionIsReversed() const {
        return list_direction_ == Direction::HORIZONTAL_REVERSED || list_direction_ == Direction::VERTICAL_REVERSED;
    }

    void setListDirection(std::string_view direction) { setListDirection(strToListDirection(direction)); }
    void setListJustify(std::string_view justify) { setListJustify(strToListJustify(justify)); }
    void setListJustifyLast(std::string_view justify) { setListJustifyLast(strToListJustify(justify)); }
    void setListCrossAlign(std::string_view align) { setListCrossAlign(strToListCrossAlign(align)); }
    void setListWrapMode(std::string_view mode) { setListWrapMode(strToListWrapMode(mode)); }

    static void apply(const Entt& e, const sol::object& obj);

    // NOLINTBEGIN(readability-identifier-naming, readability-avoid-const-params-in-decls) Not my declaration
    static void on_construct(entt::registry& registry, const entt::entity entt);
    static void on_destroy(entt::registry& registry, const entt::entity entt);
    // NOLINTEND(readability-identifier-naming, readability-avoid-const-params-in-decls)

    static Direction strToListDirection(std::string_view str);
    static Justify strToListJustify(std::string_view str);
    static CrossAlign strToListCrossAlign(std::string_view str);
    static WrapMode strToListWrapMode(std::string_view str);

  private:
    bool scrollable_ = true;
    bool pack_children_ = false;

    Direction list_direction_ = Direction::VERTICAL;
    Justify list_justify_ = Justify::START;
    Justify list_justify_last_ = Justify::START;
    CrossAlign list_cross_align_ = CrossAlign::START;
    WrapMode list_wrap_mode_ = WrapMode::NO;
    TargetBound list_cross_max_size_;
    TargetBound list_main_min_gap_;
    TargetBound list_cross_gap_;
};
}  // namespace mle::ui::comp

namespace fmt {
using mle::ui::comp::ListContainer;

template <>
struct formatter<ListContainer::Direction> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(ListContainer::Direction direction, FormatContext& ctx) const {
        switch (direction) {
            case ListContainer::Direction::HORIZONTAL:
                return format_to(ctx.out(), "HORIZONTAL");
            case ListContainer::Direction::VERTICAL:
                return format_to(ctx.out(), "VERTICAL");
            case ListContainer::Direction::HORIZONTAL_REVERSED:
                return format_to(ctx.out(), "HORIZONTAL_REVERSED");
            case ListContainer::Direction::VERTICAL_REVERSED:
                return format_to(ctx.out(), "VERTICAL_REVERSED");
            default:
                return format_to(ctx.out(), "UNKNOWN");
        }
    }
};

template <>
struct formatter<ListContainer::WrapMode> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(ListContainer::WrapMode mode, FormatContext& ctx) const {
        switch (mode) {
            case ListContainer::WrapMode::NO:
                return format_to(ctx.out(), "NO");
            case ListContainer::WrapMode::WRAP:
                return format_to(ctx.out(), "WRAP");
            case ListContainer::WrapMode::WRAP_REVERSED:
                return format_to(ctx.out(), "WRAP_REVERSED");
            default:
                return format_to(ctx.out(), "UNKNOWN");
        }
    }
};

template <>
struct formatter<ListContainer::CrossAlign> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(ListContainer::CrossAlign align, FormatContext& ctx) const {
        switch (align) {
            case ListContainer::CrossAlign::START:
                return format_to(ctx.out(), "START");
            case ListContainer::CrossAlign::CENTER:
                return format_to(ctx.out(), "CENTER");
            case ListContainer::CrossAlign::END:
                return format_to(ctx.out(), "END");
            case ListContainer::CrossAlign::STRETCH:
                return format_to(ctx.out(), "STRETCH");
            default:
                return format_to(ctx.out(), "UNKNOWN");
        }
    }
};

}  // namespace fmt
