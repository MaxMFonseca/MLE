#include "ListContainer.h"

#include "ContainerUtils.h"
#include "mle/lua/Utils.h"
#include "mle/utils/String.h"

namespace mle::ui::comp {
void ListContainer::apply(const Entt& e, const sol::object& obj) {
    auto table_r = lua::tryAs<sol::table>(obj);
    if (!table_r) {
        MLE_E("Invalid object to apply Container component at entity {}. Expected table.", e.name());
        return;
    }
    e.patchOrEmplace<ListContainer>([&](ListContainer& c) { c.set(*table_r); });
}

void ListContainer::on_construct(entt::registry& registry, const entt::entity entt) {
    if (registry.any_of<SizeProvider>(entt)) {
        MLE_W("An element shouldnt have multiple SizeProvider components.");
        return;
    }
    registry.emplace<SizeProvider>(entt, [](const Entt& e, vec2u max_size) { return e.get<comp::ListContainer>().calculateChildrenBounds(e, max_size); });
}

void ListContainer::on_destroy(entt::registry& registry, const entt::entity entt) {
    registry.remove<SizeProvider>(entt);
};

void ListContainer::set(const sol::table& table) {
    if (const auto scrollable_r = table["scrollable"]; lua::valid<bool>(scrollable_r)) {
        setScrollable(lua::as<bool>(scrollable_r));
    }
    if (const auto list_direction_r = lua::getFirstKey(table, "direction", "dir"); lua::valid<std::string>(list_direction_r)) {
        setListDirection(lua::as<std::string>(list_direction_r));
    }
    if (const auto list_justify_r = lua::getFirstKey(table, "justify"); lua::valid<std::string>(list_justify_r)) {
        setListJustify(lua::as<std::string>(list_justify_r));
    }
    if (const auto list_justify_last_r = lua::getFirstKey(table, "justify_last"); lua::valid<std::string>(list_justify_last_r)) {
        setListJustifyLast(lua::as<std::string>(list_justify_last_r));
    }
    if (const auto list_align_cross_r = lua::getFirstKey(table, "cross_align"); lua::valid<std::string>(list_align_cross_r)) {
        setListCrossAlign(lua::as<std::string>(list_align_cross_r));
    }
    if (const auto list_wrap_mode_r = lua::getFirstKey(table, "wrap"); lua::valid<std::string>(list_wrap_mode_r)) {
        setListWrapMode(lua::as<std::string>(list_wrap_mode_r));
    }
    if (const auto list_cross_max_size_r = lua::getFirstKey(table, "cross_max_size"); list_cross_max_size_r.valid()) {
        TargetBound tb{};
        tb.set(list_cross_max_size_r);
        setListCrossMaxSize(tb);
    }
    if (const auto list_main_min_gap_r = lua::getFirstKey(table, "main_min_gap", "gap"); list_main_min_gap_r.valid()) {
        TargetBound tb{};
        tb.set(list_main_min_gap_r);
        setListMainMinGap(tb);
    }
    if (const auto list_cross_gap_r = lua::getFirstKey(table, "cross_gap"); list_cross_gap_r.valid()) {
        TargetBound tb{};
        tb.set(list_cross_gap_r);
        setListCrossGap(tb);
    }
    if (const auto pack_children_r = lua::getFirstKey(table, "pack"); lua::valid<bool>(pack_children_r)) {
        setPackChildren(lua::as<bool>(pack_children_r));
    }
}

ListContainer::Direction ListContainer::strToListDirection(std::string_view str) {
    auto s = toLower(str);
    if (matchAny(s, "horizontal", "h", "row")) {
        return Direction::HORIZONTAL;
    }
    if (matchAny(s, "vertical", "v", "column", "col")) {
        return Direction::VERTICAL;
    }
    if (matchAny(s, "horizontal_reversed", "h_r", "row_reversed", "row_r")) {
        return Direction::HORIZONTAL_REVERSED;
    }
    if (matchAny(s, "vertical_reversed", "v_r", "column_reversed", "col_reversed", "col_r")) {
        return Direction::VERTICAL_REVERSED;
    }

    MLE_W("Invalid list_direction '{}' for ListContainer. Using VERTICAL.", str);
    return Direction::VERTICAL;
}

ListContainer::Justify ListContainer::strToListJustify(std::string_view str) {
    auto s = toLower(str);
    if (matchAny(s, "start", "s", "b")) {
        return JustifyInt::LineMode::START;
    }
    if (matchAny(s, "center", "centre", "c")) {
        return JustifyInt::LineMode::CENTER;
    }
    if (matchAny(s, "end", "e")) {
        return JustifyInt::LineMode::END;
    }
    if (matchAny(s, "space_between", "sb")) {
        return JustifyInt::LineMode::SPACE_BETWEEN;
    }
    if (matchAny(s, "space_around", "sa")) {
        return JustifyInt::LineMode::SPACE_AROUND;
    }
    if (matchAny(s, "space_evenly", "se")) {
        return JustifyInt::LineMode::SPACE_EVENLY;
    }

    MLE_W("Invalid list_justify '{}' for ListContainer. Using START.", str);
    return JustifyInt::LineMode::START;
}

ListContainer::CrossAlign ListContainer::strToListCrossAlign(std::string_view str) {
    auto s = toLower(str);
    if (matchAny(s, "start", "s", "b")) {
        return CrossAlign::START;
    }
    if (matchAny(s, "center", "centre", "c")) {
        return CrossAlign::CENTER;
    }
    if (matchAny(s, "end", "e")) {
        return CrossAlign::END;
    }
    if (matchAny(s, "stretch")) {
        return CrossAlign::STRETCH;
    }

    MLE_W("Invalid cross_align '{}' for ListContainer. Using START.", str);
    return CrossAlign::START;
}

ListContainer::WrapMode ListContainer::strToListWrapMode(std::string_view str) {
    auto s = toLower(str);
    if (matchAny(s, "n", "no")) {
        return WrapMode::NO;
    }
    if (matchAny(s, "y", "yes", "wrap")) {
        return WrapMode::WRAP;
    }
    if (matchAny(s, "y_r", "yes_reversed", "wrap_reversed", "wrap_r")) {
        return WrapMode::WRAP_REVERSED;
    }

    MLE_W("Invalid list_wrap_mode '{}' for ListContainer. Using NO.", str);
    return WrapMode::NO;
}

namespace {
struct ListChildCalcData {
    enum class CalcState : u8 { UNSEEN, DONE, FLEX, FIT, AR };
    bool valid = true;
    bool done = false;

    struct {
        int main_a{}, main_b{}, cross_a{}, cross_b{};
    } margin{}, border{};

    bool is_fit_main = false;
    bool is_fit_cross = false;
    bool is_main_ar_defined = false;
    bool is_cross_ar_defined = false;

    struct {
        struct {
            f32 main{};
            f32 cross{};
        } size{};
        struct {
            f32 main_a{}, main_b{}, cross_a{}, cross_b{};
        } margin{};
        struct {
            f32 main_a{}, main_b{}, cross_a{}, cross_b{};
        } border;
    } flex{};

    struct AxisTargets {
        TargetBound size{}, margin_a{}, margin_b{}, border_a{}, border_b{};
    };
    AxisTargets target_main{}, target_cross{};

    int size_main = 0;
    int size_cross = 0;
    int pos_main = 0;
    int pos_cross = 0;

    struct {
        CalcState main, cross, margin_main_a, margin_main_b, margin_cross_a, margin_cross_b, border_main_a, border_main_b, border_cross_a, border_cross_b;
    } state{};

    ListChildCalcData(const ChildBoundsCalcData& cbcd, bool main_is_x) {
        if (main_is_x) {
            target_main = {.size = cbcd.target.size.x,
                           .margin_a = cbcd.target.margin.l,
                           .margin_b = cbcd.target.margin.r,
                           .border_a = cbcd.target.border.l,
                           .border_b = cbcd.target.border.r};
            target_cross = {.size = cbcd.target.size.y,
                            .margin_a = cbcd.target.margin.t,
                            .margin_b = cbcd.target.margin.b,
                            .border_a = cbcd.target.border.t,
                            .border_b = cbcd.target.border.b};
        } else {
            target_main = {.size = cbcd.target.size.y,
                           .margin_a = cbcd.target.margin.t,
                           .margin_b = cbcd.target.margin.b,
                           .border_a = cbcd.target.border.t,
                           .border_b = cbcd.target.border.b};
            target_cross = {.size = cbcd.target.size.x,
                            .margin_a = cbcd.target.margin.l,
                            .margin_b = cbcd.target.margin.r,
                            .border_a = cbcd.target.border.l,
                            .border_b = cbcd.target.border.r};
        }
    }
};

struct ListCalculator {
    const Entt list_ew;
    const ListContainer& list;

    const bool main_is_x{};

    const int padded_size_main;
    const int padded_size_cross;
    const f32 padded_size_main_f;
    const f32 padded_size_cross_f;

    const vec2i child_max_size;
    const vec2f child_max_size_f;

    const int child_max_size_main;
    const int child_max_size_cross;
    const f32 child_max_size_main_f;
    const f32 child_max_size_cross_f;

    const vec2i root_size;

    const int root_size_main;
    const int root_size_cross;
    const f32 root_size_main_f;
    const f32 root_size_cross_f;

    const int min_gap_main = 0;
    const int cross_gap = 0;

    const bool main_wrap;

    const std::vector<entt::entity> list_children;
    std::map<entt::entity, ChildBoundsCalcData>& cbcds;
    std::vector<ListChildCalcData> list_children_data;

    ListCalculator(Entt list_ew, const ListContainer& list, vec2i padded_size, PaddingPx padding_px, std::vector<entt::entity>& list_children,
                   std::map<entt::entity, ChildBoundsCalcData>& cbcds) :
        list_ew(list_ew),
        list(list),
        main_is_x(list.getListDirection() == ListContainer::Direction::HORIZONTAL || list.getListDirection() == ListContainer::Direction::HORIZONTAL_REVERSED),
        padded_size_main(main_is_x ? padded_size.x : padded_size.y),
        padded_size_cross(main_is_x ? padded_size.y : padded_size.x),
        padded_size_main_f(as<f32>(padded_size_main)),
        padded_size_cross_f(as<f32>(padded_size_cross)),
        child_max_size(padded_size),
        child_max_size_f(child_max_size),
        child_max_size_main(main_is_x ? padded_size.x : padded_size.y),
        child_max_size_cross(calcChildMaxSizeCross(list.getListCrossMaxSize(), padded_size_cross, padded_size_cross_f,
                                                   main_is_x ? as<f32>(root_size.y) : as<f32>(root_size.x), padded_size)),
        child_max_size_main_f(as<f32>(child_max_size_main)),
        child_max_size_cross_f(as<f32>(child_max_size_cross)),
        root_size(list_ew.ui().getRootMaxSize()),
        root_size_main(main_is_x ? root_size.x : root_size.y),
        root_size_cross(main_is_x ? root_size.y : root_size.x),
        root_size_main_f(as<f32>(root_size_main)),
        root_size_cross_f(as<f32>(root_size_cross)),
        min_gap_main(calcMinGapMain(list.getListMainMinGap(), root_size_main_f, child_max_size_main_f, padded_size)),
        cross_gap(calcCrossGap(list.getListCrossGap(), root_size_cross_f, child_max_size_cross_f, padded_size)),
        main_wrap(list.getListWrapMode() != ListContainer::WrapMode::NO),
        list_children(list_children),
        cbcds(cbcds) {
        for (auto c : list_children) {
            list_children_data.emplace_back(cbcds.at(c), main_is_x);
        }

        calculateChildrenSizes();
        calculateChildrenPositions();
        finishChildrenBounds(list_ew, cbcds, list_children, padding_px, padded_size);
    }

    static int calcChildMaxSizeCross(const TargetBound& tb, int padded_size_cross, f32 padded_size_cross_f, f32 root_size_cross_f, vec2i padded_size) {
        switch (tb.type) {
            case TargetBound::Type::PX: {
                return as<int>(tb.val);
            } break;
            case TargetBound::Type::DEFAULT: {
                if (tb.val == 0.0F) {
                    return padded_size_cross;
                }
                return as<int>(padded_size_cross_f * tb.val);
            } break;
            case TargetBound::Type::RELATIVE: {
                return as<int>(padded_size_cross_f * tb.val);
            } break;
            case TargetBound::Type::ROOT: {
                return as<int>(root_size_cross_f * tb.val);
            } break;
            case TargetBound::Type::RELATIVE_W: {
                return as<int>(as<f32>(padded_size.x) * tb.val);
            } break;
            case TargetBound::Type::RELATIVE_H: {
                return as<int>(as<f32>(padded_size.y) * tb.val);
            } break;
            default: {
                MLE_W("Invalid target bound type for list cross max size: {}, treating as 0px", tb.type);
                return 0;
            } break;
        }
    }

    static int calcMinGapMain(const TargetBound& min_gap_main, const f32 root_size_main_px_f, const f32 max_size_main_px_f, const vec2i& padded_max_size_px) {
        switch (min_gap_main.type) {
            case TargetBound::Type::PX: {
                return as<int>(min_gap_main.val);
            }
            case TargetBound::Type::ROOT: {
                return as<int>(root_size_main_px_f * min_gap_main.val);
            } break;
            case TargetBound::Type::DEFAULT: {
                return as<int>(min_gap_main.val);
            } break;
            case TargetBound::Type::RELATIVE: {
                return as<int>(max_size_main_px_f * min_gap_main.val);
            } break;
            case TargetBound::Type::RELATIVE_W: {
                return as<int>(as<f32>(padded_max_size_px.x) * min_gap_main.val);
            } break;
            case TargetBound::Type::RELATIVE_H: {
                return as<int>(as<f32>(padded_max_size_px.y) * min_gap_main.val);
            } break;
            default: {
                MLE_W("Invalid min gap type: {}. Returning 0.", min_gap_main.type);
                return 0;
            } break;
        }
    }

    [[nodiscard]] int calcCrossGap(const TargetBound& tb, const f32 root_size_cross_px_f, const f32 max_size_cross_px_f,
                                   const vec2i& padded_max_size_px) const {
        switch (tb.type) {
            case TargetBound::Type::PX: {
                return as<int>(tb.val);
            }
            case TargetBound::Type::ROOT: {
                return as<int>(root_size_cross_px_f * tb.val);
            } break;
            case TargetBound::Type::DEFAULT: {
                return as<int>(child_max_size_cross_f * tb.val);
            } break;
            case TargetBound::Type::RELATIVE: {
                return as<int>(max_size_cross_px_f * tb.val);
            } break;
            case TargetBound::Type::RELATIVE_W: {
                return as<int>(as<f32>(padded_max_size_px.x) * tb.val);
            } break;
            case TargetBound::Type::RELATIVE_H: {
                return as<int>(as<f32>(padded_max_size_px.y) * tb.val);
            } break;
            default: {
                MLE_W("Invalid cross gap type: {}. Returning 0.", tb.type);
                return 0;
            } break;
        }
    }

    bool calculateChildMarginSize(ListChildCalcData::CalcState& calc_state, int& ret_px, f32& ret_flex, const TargetBound& target_margin,
                                  const bool is_main) const {
        using CalcState = ListChildCalcData::CalcState;
        switch (target_margin.type) {
            case TargetBound::Type::PX: {
                ret_px = as<int>(target_margin.val);
                calc_state = CalcState::DONE;
            } break;
            case TargetBound::Type::DEFAULT:
            case TargetBound::Type::RELATIVE: {
                ret_px = is_main ? as<int>(child_max_size_main_f * target_margin.val) : as<int>(child_max_size_cross_f * target_margin.val);
                calc_state = CalcState::DONE;
            } break;
            case TargetBound::Type::ROOT: {
                ret_px = is_main ? as<int>(root_size_main_f * target_margin.val) : as<int>(root_size_cross_f * target_margin.val);
                calc_state = CalcState::DONE;
            } break;
            case TargetBound::Type::RELATIVE_W: {
                ret_px = as<int>(child_max_size_f.x * target_margin.val);
                calc_state = CalcState::DONE;
            } break;
            case TargetBound::Type::RELATIVE_H: {
                ret_px = as<int>(child_max_size_f.y * target_margin.val);
                calc_state = CalcState::DONE;
            } break;
            case TargetBound::Type::FLEX_SHARE: {
                ret_flex = target_margin.val;
                calc_state = CalcState::FLEX;
            } break;
            default: {
                MLE_W("Invalid margin target bound type for margin calculation: {}, treating as 0px", target_margin.type);
                ret_px = 0;
                calc_state = CalcState::DONE;
            } break;
        };
        return calc_state == CalcState::DONE;
    };

    bool calculateChildContentSize(ListChildCalcData::CalcState& calc_state, int& ret_px, f32& ret_flex, const TargetBound& target_size, const bool is_main,
                                   const bool has_size_provider, const f32 aspect_ratio) const {
        using CalcState = ListChildCalcData::CalcState;
        switch (target_size.type) {
            case TargetBound::Type::PX: {
                ret_px = as<int>(target_size.val);
                calc_state = CalcState::DONE;
            } break;
            case TargetBound::Type::RELATIVE: {
                ret_px = is_main ? as<int>(child_max_size_main_f * target_size.val) : as<int>(child_max_size_cross_f * target_size.val);
                calc_state = CalcState::DONE;
            } break;
            case TargetBound::Type::ROOT: {
                ret_px = is_main ? as<int>(root_size_main_f * target_size.val) : as<int>(root_size_cross_f * target_size.val);
                calc_state = CalcState::DONE;
            } break;
            case TargetBound::Type::RELATIVE_W: {
                ret_px = as<int>(child_max_size_f.x * target_size.val);
                calc_state = CalcState::DONE;
            } break;
            case TargetBound::Type::RELATIVE_H: {
                ret_px = as<int>(child_max_size_f.y * target_size.val);
                calc_state = CalcState::DONE;
            } break;
            case TargetBound::Type::FLEX_SHARE: {
                ret_flex = target_size.val;
                calc_state = CalcState::FLEX;
            } break;
            case TargetBound::Type::FIT: {
                calc_state = CalcState::FIT;
            } break;
            default: {
                if (target_size.val > 0) {
                    if (main_wrap && is_main) {
                        ret_px = as<int>(target_size.val * child_max_size_main_f);
                        calc_state = CalcState::DONE;
                    } else {
                        ret_flex = target_size.val;
                        calc_state = CalcState::FLEX;
                    }
                } else if (aspect_ratio > 0) {
                    calc_state = CalcState::AR;
                } else if (has_size_provider) {
                    calc_state = CalcState::FIT;
                } else {
                    if (main_wrap && is_main) {
                        ret_px = child_max_size_main;
                        calc_state = CalcState::DONE;
                    } else {
                        ret_flex = 1;
                        calc_state = CalcState::FLEX;
                    }
                }
            } break;
        };
        return calc_state == CalcState::DONE;
    }

    void finishFlexCross(ListChildCalcData& cld) const {
        using CalcState = ListChildCalcData::CalcState;
        f32 flex_acc_cross = cld.flex.size.cross + cld.flex.margin.cross_a + cld.flex.margin.cross_b + cld.flex.border.cross_a + cld.flex.border.cross_b;
        if (flex_acc_cross > 0.0F) {
            int px_cross = child_max_size_cross - cld.size_cross - cld.margin.cross_a - cld.margin.cross_b - cld.border.cross_a - cld.border.cross_b;
            px_cross = std::max(0, px_cross);
            f32 flex_share_size = as<f32>(px_cross) / std::max(1.F, flex_acc_cross);
            if (cld.state.cross == CalcState::FLEX) {
                cld.size_cross = as<int>(cld.flex.size.cross * flex_share_size);
                cld.state.cross = CalcState::DONE;
            }
            if (cld.state.margin_cross_a == CalcState::FLEX) {
                cld.margin.cross_a = as<int>(cld.flex.margin.cross_a * flex_share_size);
                cld.state.margin_cross_a = CalcState::DONE;
            }
            if (cld.state.margin_cross_b == CalcState::FLEX) {
                cld.margin.cross_b = as<int>(cld.flex.margin.cross_b * flex_share_size);
                cld.state.margin_cross_b = CalcState::DONE;
            }
            if (cld.state.border_cross_a == CalcState::FLEX) {
                cld.border.cross_a = as<int>(cld.flex.border.cross_a * flex_share_size);
                cld.state.border_cross_a = CalcState::DONE;
            }
            if (cld.state.border_cross_b == CalcState::FLEX) {
                cld.border.cross_b = as<int>(cld.flex.border.cross_b * flex_share_size);
                cld.state.border_cross_b = CalcState::DONE;
            }
        }
    }

    // NOLINTNEXTLINE(readability-function-cognitive-complexity) Not that complex
    void calculateChildrenSizes() {
        f32 flex_acc_main = 0;
        int px_acc_main = 0;

        std::map<entt::entity, ListChildCalcData> children_sizes_data;

        for (usize i = 0; i < list_children.size(); ++i) {
            auto c = list_children[i];
            auto centt = list_ew.derive(c);
            auto& cbcd = cbcds.at(c);
            auto& lccd = list_children_data.at(i);

            bool done = calculateChildMarginSize(lccd.state.margin_main_a, lccd.margin.main_a, lccd.flex.margin.main_a, lccd.target_main.margin_a, true);
            done &= calculateChildMarginSize(lccd.state.margin_main_b, lccd.margin.main_b, lccd.flex.margin.main_b, lccd.target_main.margin_b, true);

            if ((lccd.target_cross.margin_a.type != TargetBound::Type::DEFAULT && lccd.target_cross.margin_a.val != 0.0F) ||
                (lccd.target_cross.margin_b.type != TargetBound::Type::DEFAULT && lccd.target_cross.margin_b.val != 0.0F)) {
                MLE_W("Non-default cross margin on list child is not supported and will be ignored.");
            }

            done &= calculateChildMarginSize(lccd.state.border_main_a, lccd.border.main_a, lccd.flex.border.main_a, lccd.target_main.border_a, true);
            done &= calculateChildMarginSize(lccd.state.border_main_b, lccd.border.main_b, lccd.flex.border.main_b, lccd.target_main.border_b, true);
            done &= calculateChildMarginSize(lccd.state.border_cross_a, lccd.border.cross_a, lccd.flex.border.cross_a, lccd.target_cross.border_a, false);
            done &= calculateChildMarginSize(lccd.state.border_cross_b, lccd.border.cross_b, lccd.flex.border.cross_b, lccd.target_cross.border_b, false);

            done &= calculateChildContentSize(lccd.state.main, lccd.size_main, lccd.flex.size.main, lccd.target_main.size, true, cbcd.size_provider != nullptr,
                                              cbcd.target.aspect_ratio);

            if (list.getListCrossAlign() != ListContainer::CrossAlign::STRETCH) {
                done &= calculateChildContentSize(lccd.state.cross, lccd.size_cross, lccd.flex.size.cross, lccd.target_cross.size, false,
                                                  cbcd.size_provider != nullptr, cbcd.target.aspect_ratio);
            } else {
                lccd.size_cross = 0;
                lccd.flex.size.cross = 1.0F;
                lccd.state.cross = ListChildCalcData::CalcState::FLEX;
            }

            switch (list.getListCrossAlign()) {
                case ListContainer::CrossAlign::CENTER: {
                    f32 flex_cross_acc = lccd.flex.size.cross + lccd.flex.border.cross_a + lccd.flex.border.cross_b;
                    if (flex_cross_acc < 1) {
                        f32 one_minux_flex_cross_acc = 1 - flex_cross_acc;
                        lccd.flex.margin.cross_a = one_minux_flex_cross_acc / 2;
                        lccd.flex.margin.cross_b = lccd.flex.margin.cross_a;
                        lccd.state.margin_cross_a = ListChildCalcData::CalcState::FLEX;
                        lccd.state.margin_cross_b = ListChildCalcData::CalcState::FLEX;
                        done = false;
                    } else {
                        lccd.state.margin_cross_a = ListChildCalcData::CalcState::DONE;
                        lccd.state.margin_cross_b = ListChildCalcData::CalcState::DONE;
                    }
                } break;
                case ListContainer::CrossAlign::END: {
                    f32 flex_cross_acc = lccd.flex.size.cross + lccd.flex.border.cross_a + lccd.flex.border.cross_b;
                    if (flex_cross_acc < 1) {
                        f32 one_minux_flex_cross_acc = 1 - flex_cross_acc;
                        lccd.flex.margin.cross_a = one_minux_flex_cross_acc;
                        lccd.flex.margin.cross_b = 0;
                        lccd.state.margin_cross_a = ListChildCalcData::CalcState::FLEX;
                        lccd.state.margin_cross_b = ListChildCalcData::CalcState::FLEX;
                        done = false;
                    } else {
                        lccd.state.margin_cross_a = ListChildCalcData::CalcState::DONE;
                        lccd.state.margin_cross_b = ListChildCalcData::CalcState::DONE;
                    }
                } break;
                default: {
                    lccd.state.margin_cross_a = ListChildCalcData::CalcState::DONE;
                    lccd.state.margin_cross_b = ListChildCalcData::CalcState::DONE;
                } break;
            }

            if (done) {
                lccd.done = true;
                flex_acc_main += lccd.flex.size.main + lccd.flex.margin.main_a + lccd.flex.margin.main_b + lccd.flex.border.main_a + lccd.flex.border.main_b;
                px_acc_main += lccd.size_main + lccd.margin.main_a + lccd.margin.main_b + lccd.border.main_a + lccd.border.main_b;
                continue;
            }

            if (lccd.state.main == ListChildCalcData::CalcState::FLEX) {
                flex_acc_main += lccd.flex.size.main + lccd.flex.margin.main_a + lccd.flex.margin.main_b + lccd.flex.border.main_a + lccd.flex.border.main_b;
                px_acc_main += lccd.size_main + lccd.margin.main_a + lccd.margin.main_b + lccd.border.main_a + lccd.border.main_b;
                continue;
            }

            if (lccd.state.main == ListChildCalcData::CalcState::FIT && lccd.state.cross == ListChildCalcData::CalcState::FIT) {
                int fit_max_size_main = child_max_size_main - lccd.margin.main_a - lccd.margin.main_b - lccd.border.main_a - lccd.border.main_b;
                int fit_max_size_cross = child_max_size_cross - lccd.margin.cross_a - lccd.margin.cross_b - lccd.border.cross_a - lccd.border.cross_b;

                if (fit_max_size_cross <= 0 || fit_max_size_main <= 0) {
                    MLE_E("Child cannot FIT in the given max size");
                    lccd.valid = false;
                    continue;
                }

                vec2u p_max_size =
                    main_is_x ? vec2u{as<u32>(fit_max_size_main), as<u32>(fit_max_size_cross)} : vec2u{as<u32>(fit_max_size_cross), as<u32>(fit_max_size_main)};
                vec2u size_from_provider = cbcd.size_provider->call(centt, p_max_size);
                u32 main_size_from_provider = main_is_x ? size_from_provider.x : size_from_provider.y;
                u32 cross_size_from_provider = main_is_x ? size_from_provider.y : size_from_provider.x;

                lccd.size_main = as<int>(main_size_from_provider);
                lccd.state.main = ListChildCalcData::CalcState::DONE;

                lccd.size_cross = as<int>(cross_size_from_provider);
                lccd.state.cross = ListChildCalcData::CalcState::DONE;
            }

            if (lccd.state.cross == ListChildCalcData::CalcState::FIT) {
                if (lccd.state.main == ListChildCalcData::CalcState::DONE) {
                    int fit_max_size_cross = child_max_size_cross - lccd.margin.cross_a - lccd.margin.cross_b - lccd.border.cross_a - lccd.border.cross_b;
                    if (fit_max_size_cross <= 0) {
                        MLE_E("Child cannot FIT in the given max cross size");
                        lccd.valid = false;
                        continue;
                    }

                    vec2u p_max_size =
                        main_is_x ? vec2u{as<u32>(lccd.size_main), as<u32>(fit_max_size_cross)} : vec2u{as<u32>(fit_max_size_cross), as<u32>(lccd.size_main)};
                    vec2u size_from_provider = cbcd.size_provider->call(centt, p_max_size);
                    u32 cross_size_from_provider = main_is_x ? size_from_provider.y : size_from_provider.x;

                    lccd.size_cross = as<int>(cross_size_from_provider);
                    lccd.state.cross = ListChildCalcData::CalcState::DONE;
                } else if (lccd.state.main == ListChildCalcData::CalcState::AR) {
                    MLE_E("Cannot mix FIT and AR");
                    lccd.valid = false;
                    continue;
                }
            }

            if (lccd.state.cross == ListChildCalcData::CalcState::AR) {
                if (lccd.state.main == ListChildCalcData::CalcState::DONE) {
                    if (main_is_x) {
                        lccd.size_cross = as<int>(as<f32>(lccd.size_main) / cbcd.target.aspect_ratio);
                    } else {
                        lccd.size_cross = as<int>(as<f32>(lccd.size_main) * cbcd.target.aspect_ratio);
                    }
                    lccd.state.cross = ListChildCalcData::CalcState::DONE;
                } else if (lccd.state.main == ListChildCalcData::CalcState::FIT) {
                    MLE_E("Cannot mix FIT and AR");
                    lccd.valid = false;
                    continue;
                } else if (lccd.state.main == ListChildCalcData::CalcState::AR) {
                    MLE_E("Cannot have AR on both axis.");
                    lccd.valid = false;
                    continue;
                }
            }

            finishFlexCross(lccd);

            if (lccd.state.main == ListChildCalcData::CalcState::FIT) {
                if (lccd.state.cross == ListChildCalcData::CalcState::DONE) {
                    int fit_max_size_main = child_max_size_main - lccd.margin.main_a - lccd.margin.main_b - lccd.border.main_a - lccd.border.main_b;
                    if (fit_max_size_main <= 0) {
                        MLE_E("Child cannot FIT in the given max main size");
                        lccd.valid = false;
                        continue;
                    }

                    vec2u p_max_size =
                        main_is_x ? vec2u{as<u32>(fit_max_size_main), as<u32>(lccd.size_cross)} : vec2u{as<u32>(lccd.size_cross), as<u32>(fit_max_size_main)};
                    vec2u size_from_provider = cbcd.size_provider->call(centt, p_max_size);
                    u32 main_size_from_provider = main_is_x ? size_from_provider.x : size_from_provider.y;

                    lccd.size_main = as<int>(main_size_from_provider);
                    lccd.state.main = ListChildCalcData::CalcState::DONE;
                } else {
                    MLE_E("This should be unreachable due to previous checks. Main state: FIT, Cross state:{}", (int)lccd.state.cross);
                    lccd.valid = false;
                    continue;
                }
            }

            if (lccd.state.main == ListChildCalcData::CalcState::AR) {
                if (lccd.state.cross == ListChildCalcData::CalcState::DONE) {
                    if (main_is_x) {
                        lccd.size_main = as<int>(as<f32>(lccd.size_cross) * cbcd.target.aspect_ratio);
                    } else {
                        lccd.size_main = as<int>(as<f32>(lccd.size_cross) / cbcd.target.aspect_ratio);
                    }
                    lccd.state.main = ListChildCalcData::CalcState::DONE;
                } else {
                    MLE_E("This should be unreachable due to previous checks. Main state: AR/FIT, Cross state:{}", (int)lccd.state.cross);
                    lccd.valid = false;
                    continue;
                }
            }

            flex_acc_main += lccd.flex.size.main + lccd.flex.margin.main_a + lccd.flex.margin.main_b + lccd.flex.border.main_a + lccd.flex.border.main_b;
            px_acc_main += lccd.size_main + lccd.margin.main_a + lccd.margin.main_b + lccd.border.main_a + lccd.border.main_b;
        }

        px_acc_main += list_children.size() > 0 ? min_gap_main * (as<int>(list_children.size()) - 1) : 0;

        int remaining_px_main = as<int>(padded_size_main) - px_acc_main;
        remaining_px_main = std::max(0, remaining_px_main);
        f32 flex_share_main = (flex_acc_main > 0.0F) ? as<f32>(remaining_px_main) / std::max(1.F, flex_acc_main) : 0.0F;

        for (usize i = 0; i < list_children.size(); ++i) {
            auto c = list_children[i];
            // auto centt = list_ew.derive(c);
            auto& cbcd = cbcds.at(c);
            auto& lccd = list_children_data.at(i);

            if (!lccd.done) {
                if (lccd.state.main == ListChildCalcData::CalcState::FLEX) {
                    lccd.size_main = as<int>(lccd.flex.size.main * flex_share_main);
                    lccd.state.main = ListChildCalcData::CalcState::DONE;
                }
                if (lccd.state.margin_main_a == ListChildCalcData::CalcState::FLEX) {
                    lccd.margin.main_a = as<int>(lccd.flex.margin.main_a * flex_share_main);
                    lccd.state.margin_main_a = ListChildCalcData::CalcState::DONE;
                }
                if (lccd.state.margin_main_b == ListChildCalcData::CalcState::FLEX) {
                    lccd.margin.main_b = as<int>(lccd.flex.margin.main_b * flex_share_main);
                    lccd.state.margin_main_b = ListChildCalcData::CalcState::DONE;
                }
                if (lccd.state.border_main_a == ListChildCalcData::CalcState::FLEX) {
                    lccd.border.main_a = as<int>(lccd.flex.border.main_a * flex_share_main);
                    lccd.state.border_main_a = ListChildCalcData::CalcState::DONE;
                }
                if (lccd.state.border_main_b == ListChildCalcData::CalcState::FLEX) {
                    lccd.border.main_b = as<int>(lccd.flex.border.main_b * flex_share_main);
                    lccd.state.border_main_b = ListChildCalcData::CalcState::DONE;
                }
                // if cross is ar
                if (lccd.state.cross == ListChildCalcData::CalcState::AR) {
                    if (main_is_x) {
                        lccd.size_cross = as<int>(as<f32>(lccd.size_main) / cbcd.target.aspect_ratio);
                    } else {
                        lccd.size_cross = as<int>(as<f32>(lccd.size_main) * cbcd.target.aspect_ratio);
                    }
                    lccd.state.cross = ListChildCalcData::CalcState::DONE;
                }

                if (lccd.state.cross == ListChildCalcData::CalcState::FIT) {
                    int fit_max_size_cross = child_max_size_cross - lccd.margin.cross_a - lccd.margin.cross_b - lccd.border.cross_a - lccd.border.cross_b;
                    if (fit_max_size_cross <= 0) {
                        MLE_E("Child cannot FIT in the given max cross size");
                        lccd.valid = false;
                        continue;
                    }

                    vec2u p_max_size =
                        main_is_x ? vec2u{as<u32>(lccd.size_main), as<u32>(fit_max_size_cross)} : vec2u{as<u32>(fit_max_size_cross), as<u32>(lccd.size_main)};
                    vec2u size_from_provider = cbcd.size_provider->call(list_ew.derive(c), p_max_size);
                    u32 cross_size_from_provider = main_is_x ? size_from_provider.y : size_from_provider.x;

                    lccd.size_cross = as<int>(cross_size_from_provider);
                    lccd.state.cross = ListChildCalcData::CalcState::DONE;
                }

                finishFlexCross(lccd);

                lccd.done = lccd.state.main == ListChildCalcData::CalcState::DONE && lccd.state.cross == ListChildCalcData::CalcState::DONE &&
                            lccd.state.margin_main_a == ListChildCalcData::CalcState::DONE && lccd.state.margin_main_b == ListChildCalcData::CalcState::DONE &&
                            lccd.state.margin_cross_a == ListChildCalcData::CalcState::DONE &&
                            lccd.state.margin_cross_b == ListChildCalcData::CalcState::DONE && lccd.state.border_main_a == ListChildCalcData::CalcState::DONE &&
                            lccd.state.border_main_b == ListChildCalcData::CalcState::DONE && lccd.state.border_cross_a == ListChildCalcData::CalcState::DONE &&
                            lccd.state.border_cross_b == ListChildCalcData::CalcState::DONE;

                if (!lccd.done) {
                    MLE_E("Could not resolve all size/margin/border for list child {}. Please fixe me!", list_ew.derive(c).fullName());
                    MLE_E(
                        "state.main: {}, state.cross: {}, state.margin_main_a: {}, state.margin_main_b: {}, state.margin_cross_a: {}, "
                        "state.margin_cross_b: "
                        "{}, "
                        "state.border_main_a: {}, state.border_main_b: {}, state.border_cross_a: {}, state.border_cross_b: {}",
                        (int)lccd.state.main, (int)lccd.state.cross, (int)lccd.state.margin_main_a, (int)lccd.state.margin_main_b,
                        (int)lccd.state.margin_cross_a, (int)lccd.state.margin_cross_b, (int)lccd.state.border_main_a, (int)lccd.state.border_main_b,
                        (int)lccd.state.border_cross_a, (int)lccd.state.border_cross_b);
                    lccd.valid = false;
                    continue;
                }
            }

            if (main_is_x) {
                cbcd.new_size.x = lccd.size_main;
                cbcd.new_size.y = lccd.size_cross;
                cbcd.new_margin.l = lccd.margin.main_a;
                cbcd.new_margin.r = lccd.margin.main_b;
                cbcd.new_margin.t = lccd.margin.cross_a;
                cbcd.new_margin.b = lccd.margin.cross_b;
                cbcd.new_border.l = lccd.border.main_a;
                cbcd.new_border.r = lccd.border.main_b;
                cbcd.new_border.t = lccd.border.cross_a;
                cbcd.new_border.b = lccd.border.cross_b;
            } else {
                cbcd.new_size.y = lccd.size_main;
                cbcd.new_size.x = lccd.size_cross;
                cbcd.new_margin.t = lccd.margin.main_a;
                cbcd.new_margin.b = lccd.margin.main_b;
                cbcd.new_margin.l = lccd.margin.cross_a;
                cbcd.new_margin.r = lccd.margin.cross_b;
                cbcd.new_border.t = lccd.border.main_a;
                cbcd.new_border.b = lccd.border.main_b;
                cbcd.new_border.l = lccd.border.cross_a;
                cbcd.new_border.r = lccd.border.cross_b;
            }
        }
    }

    void calculateChildrenPositions() {
        std::vector<int> children_main_sizes;
        for (auto c : list_children) {
            const auto& cbcd = cbcds.at(c);
            auto total_margin_border_main = main_is_x ? (cbcd.new_margin.l + cbcd.new_margin.r + cbcd.new_border.l + cbcd.new_border.r)
                                                      : (cbcd.new_margin.t + cbcd.new_margin.b + cbcd.new_border.t + cbcd.new_border.b);
            auto total_size_main = main_is_x ? cbcd.new_size.x : cbcd.new_size.y;
            total_size_main += total_margin_border_main;
            children_main_sizes.push_back(total_size_main);
        }

        const auto main_reversed = list.getListDirectionIsReversed();
        const auto cross_reversed = list.getListWrapMode() == ListContainer::WrapMode::WRAP_REVERSED;

        const auto container_origin_main = main_reversed ? padded_size_main : 0;
        const auto container_origin_cross = cross_reversed ? padded_size_cross : 0;

        auto current_pos_cross = container_origin_cross;
        if (cross_reversed) {
            current_pos_cross -= child_max_size_cross;
        }

        switch (list.getListWrapMode()) {
            case ListContainer::WrapMode::NO: {
                auto line =
                    JustifyInt::justifyUntilOverflow(children_main_sizes, min_gap_main, list.getListJustify(), list.getListJustify(), child_max_size_main);
                if (list.isScrollable() && line.size() != list_children.size()) {
                    line = JustifyInt::justifyLineStart(children_main_sizes, min_gap_main);
                }
                for (usize i = 0; i < line.size(); i++) {
                    auto centt = Entt{list_ew.ui(), list_children[i]};
                    auto& cbcd = cbcds.at(centt.e());
                    auto pos_main_beg = line[i];
                    auto c_size_main = children_main_sizes[i];
                    auto pos_main_end = pos_main_beg + c_size_main;
                    auto new_pos_main = container_origin_main + (main_reversed ? -pos_main_end : pos_main_beg);

                    if (main_is_x) {
                        cbcd.new_position.x = new_pos_main - cbcd.new_margin.l - cbcd.new_border.l;
                        cbcd.new_position.y = current_pos_cross + cbcd.new_margin.t + cbcd.new_border.t;
                    } else {
                        cbcd.new_position.x = current_pos_cross + cbcd.new_margin.l + cbcd.new_border.l;
                        cbcd.new_position.y = new_pos_main - cbcd.new_margin.t - cbcd.new_border.t;
                    }
                }
            } break;
            case ListContainer::WrapMode::WRAP_REVERSED:
            case ListContainer::WrapMode::WRAP: {
                auto lines = JustifyInt::wrap(children_main_sizes, min_gap_main, list.getListJustify(), list.getListJustifyLast(), child_max_size_main);
                usize line_index = 0;
                usize child_index_in_line = 0;
                for (usize i = 0; i < list_children.size(); i++) {
                    auto centt = Entt{list_ew.ui(), list_children[i]};
                    auto& cbcd = cbcds.at(centt.e());
                    auto pos_main_beg = lines[line_index][child_index_in_line];
                    auto c_size_main = children_main_sizes[i];
                    auto pos_main_end = pos_main_beg + c_size_main;
                    auto new_pos_main = container_origin_main + (main_reversed ? -pos_main_end : pos_main_beg);

                    if (main_is_x) {
                        cbcd.new_position.x = new_pos_main - cbcd.new_margin.l - cbcd.new_border.l;
                        cbcd.new_position.y = current_pos_cross + cbcd.new_margin.t + cbcd.new_border.t;
                    } else {
                        cbcd.new_position.x = current_pos_cross + cbcd.new_margin.l + cbcd.new_border.l;
                        cbcd.new_position.y = new_pos_main - cbcd.new_margin.t - cbcd.new_border.t;
                    }

                    child_index_in_line++;
                    if (child_index_in_line >= lines[line_index].size()) {
                        line_index++;
                        child_index_in_line = 0;
                        current_pos_cross += cross_reversed ? -(child_max_size_cross + cross_gap) : (child_max_size_cross + cross_gap);
                    }
                }
            } break;
            default: {
                MLE_UNREACHABLE_LOG("Invalid wrap mode: {}", (int)list.getListWrapMode());
            } break;
        }
    }
};
}  // namespace

[[nodiscard]] vec2u ListContainer::calculateChildrenBounds(const Entt& e, vec2u max_size) const {
    auto& self_rel = e.getRelationship();
    auto all_children = self_rel.getChildren(e);

    std::vector<entt::entity> enabled_children;
    for (auto child_entt : all_children) {
        auto centt = e.derive(child_entt);
        if (centt.has<DisabledFlag>()) {
            continue;
        }
        enabled_children.push_back(child_entt);
    }

    std::map<entt::entity, ChildBoundsCalcData> cbcds;
    for (const auto& c : enabled_children) {
        Entt centt{e.ui(), c};
        cbcds.emplace(c, centt);
    }

    const auto* padding_comp = e.tryGet<comp::TargetPadding>();
    PaddingPx padding_result{};
    if (padding_comp) {
        padding_result = padding_comp->calc(e.ui(), max_size);
    }
    vec2i padded_max_size = padding_result.removeFrom(max_size);

    ListCalculator list_calculator{e, *this, padded_max_size, padding_result, enabled_children, cbcds};

    auto children_max_min = findChildrenMaxMin(cbcds, pack_children_);

    for (const auto& [_, cbcd] : cbcds) {
        if (pack_children_) {
            cbcd.bounds.parent_px.setPosX(cbcd.bounds.parent_px.left() - children_max_min.first.x + padding_result.l);
            cbcd.bounds.parent_px.setPosY(cbcd.bounds.parent_px.top() - children_max_min.first.y + padding_result.t);
        }
    }

    vec2u final_size = pack_children_ ? vec2u{children_max_min.second.x - children_max_min.first.x + padding_result.l + padding_result.r,
                                              children_max_min.second.y - children_max_min.first.y + padding_result.t + padding_result.b}
                                      : vec2u{children_max_min.second.x + padding_result.r, children_max_min.second.y + padding_result.b};

    return final_size;
}
}  // namespace mle::ui::comp
