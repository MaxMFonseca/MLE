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

    [[nodiscard]] vec2u calculateChildrenBounds(const Entt& e, vec2u max_size) const;

    void set(const sol::table& table);

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
    void setListDirection(ListDirection direction) { list_direction_ = direction; }
    [[nodiscard]] ListDirection getListDirection() const { return list_direction_; }
    void setListJustify(ListJustify justify) { list_justify_ = justify; }
    [[nodiscard]] ListJustify getListJustify() const { return list_justify_; }
    void setListJustifyLast(ListJustify justify) { list_justify_last_ = justify; }
    [[nodiscard]] ListJustify getListJustifyLast() const { return list_justify_last_; }
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

    [[nodiscard]] bool getListDirectionIsReversed() const {
        return list_direction_ == ListDirection::HORIZONTAL_REVERSED || list_direction_ == ListDirection::VERTICAL_REVERSED;
    }

    void setType(std::string_view type) { setType(strToType(type)); }
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

    static Type strToType(std::string_view str);
    static ListDirection strToListDirection(std::string_view str);
    static ListJustify strToListJustify(std::string_view str);
    static ListCrossAlign strToListCrossAlign(std::string_view str);
    static ListWrapMode strToListWrapMode(std::string_view str);

  private:
    Type type_ = Type::HYBRID;
    bool scrollable_ = true;
    vec2i offset_ = {0, 0};

    ListDirection list_direction_ = ListDirection::VERTICAL;
    ListJustify list_justify_ = ListJustify::START;
    ListJustify list_justify_last_ = ListJustify::START;
    ListCrossAlign list_cross_align_ = ListCrossAlign::START;
    ListWrapMode list_wrap_mode_ = ListWrapMode::NO;
    TargetBound list_cross_max_size_;
    TargetBound list_main_min_gap_;
    TargetBound list_cross_gap_;
};
}  // namespace mle::ui::comp

namespace fmt {
template <>
struct formatter<mle::ui::comp::Container::Type> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::ui::comp::Container::Type type, FormatContext& ctx) const {
        switch (type) {
            case mle::ui::comp::Container::Type::HYBRID:
                return format_to(ctx.out(), "HYBRID");
            case mle::ui::comp::Container::Type::LIST:
                return format_to(ctx.out(), "LIST");
            case mle::ui::comp::Container::Type::FREE:
                return format_to(ctx.out(), "FREE");
            default:
                return format_to(ctx.out(), "UNKNOWN");
        }
    }
};

template <>
struct formatter<mle::ui::comp::Container::ListDirection> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::ui::comp::Container::ListDirection direction, FormatContext& ctx) const {
        switch (direction) {
            case mle::ui::comp::Container::ListDirection::HORIZONTAL:
                return format_to(ctx.out(), "HORIZONTAL");
            case mle::ui::comp::Container::ListDirection::VERTICAL:
                return format_to(ctx.out(), "VERTICAL");
            case mle::ui::comp::Container::ListDirection::HORIZONTAL_REVERSED:
                return format_to(ctx.out(), "HORIZONTAL_REVERSED");
            case mle::ui::comp::Container::ListDirection::VERTICAL_REVERSED:
                return format_to(ctx.out(), "VERTICAL_REVERSED");
            default:
                return format_to(ctx.out(), "UNKNOWN");
        }
    }
};

template <>
struct formatter<mle::ui::comp::Container::ListWrapMode> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::ui::comp::Container::ListWrapMode mode, FormatContext& ctx) const {
        switch (mode) {
            case mle::ui::comp::Container::ListWrapMode::NO:
                return format_to(ctx.out(), "NO");
            case mle::ui::comp::Container::ListWrapMode::WRAP:
                return format_to(ctx.out(), "WRAP");
            case mle::ui::comp::Container::ListWrapMode::WRAP_REVERSED:
                return format_to(ctx.out(), "WRAP_REVERSED");
            default:
                return format_to(ctx.out(), "UNKNOWN");
        }
    }
};

template <>
struct formatter<mle::ui::comp::Container::ListCrossAlign> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::ui::comp::Container::ListCrossAlign align, FormatContext& ctx) const {
        switch (align) {
            case mle::ui::comp::Container::ListCrossAlign::START:
                return format_to(ctx.out(), "START");
            case mle::ui::comp::Container::ListCrossAlign::CENTER:
                return format_to(ctx.out(), "CENTER");
            case mle::ui::comp::Container::ListCrossAlign::END:
                return format_to(ctx.out(), "END");
            case mle::ui::comp::Container::ListCrossAlign::STRETCH:
                return format_to(ctx.out(), "STRETCH");
            default:
                return format_to(ctx.out(), "UNKNOWN");
        }
    }
};

}  // namespace fmt
