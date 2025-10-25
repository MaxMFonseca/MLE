#include "Container.h"

#include <ranges>
#include <string>

#include "mle/lua/Utils.h"
#include "mle/ui/Entt.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Bounds.h"
#include "mle/utils/ECS.h"
#include "mle/utils/Justify.h"
#include "mle/utils/String.h"

namespace mle::ui::comp {
void Container::apply(const Entt& e, const sol::object& obj) {
    auto table_r = lua::tryAs<sol::table>(obj);
    if (!table_r) {
        MLE_E("Invalid object to apply Container component at entity {}. Expected table.", e.name());
        return;
    }
    e.patchOrEmplace<Container>([&](Container& c) { c.set(*table_r); });
}

void Container::set(const sol::table& table) {
    if (const auto type_r = lua::getFirstKey(table, "type"); lua::valid<std::string>(type_r)) {
        setType(lua::as<std::string>(type_r));
    }
    if (const auto scrollable_r = table["scrollable"]; lua::valid<bool>(scrollable_r)) {
        setScrollable(lua::as<bool>(scrollable_r));
    }
    if (const auto offset_r = table["offset"]; offset_r.valid()) {
        setOffset(lua::as<vec2i>(offset_r));
    }
    if (const auto offset_x_r = table["offset_x"]; lua::valid<i32>(offset_x_r)) {
        setOffsetX(lua::as<i32>(offset_x_r));
    }
    if (const auto offset_y_r = table["offset_y"]; lua::valid<i32>(offset_y_r)) {
        setOffsetY(lua::as<i32>(offset_y_r));
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
    if (const auto list_main_min_gap_r = lua::getFirstKey(table, "main_min_gap"); list_main_min_gap_r.valid()) {
        TargetBound tb{};
        tb.set(list_main_min_gap_r);
        setListMainMinGap(tb);
    }
    if (const auto list_cross_gap_r = lua::getFirstKey(table, "cross_gap"); list_cross_gap_r.valid()) {
        TargetBound tb{};
        tb.set(list_cross_gap_r);
        setListCrossGap(tb);
    }
}

Container::Type Container::strToType(std::string_view str) {
    auto s = toLower(str);
    if (matchAny(s, "hybrid")) {
        return Type::HYBRID;
    }
    if (matchAny(s, "list")) {
        return Type::LIST;
    }
    if (matchAny(s, "free")) {
        return Type::FREE;
    }

    MLE_W("Invalid container type '{}' for Container. Using HYBRID.", str);
    return Type::HYBRID;
}

Container::ListDirection Container::strToListDirection(std::string_view str) {
    auto s = toLower(str);
    if (matchAny(s, "horizontal", "h", "row", "cols")) {
        return ListDirection::HORIZONTAL;
    }
    if (matchAny(s, "vertical", "v", "col", "rows")) {
        return ListDirection::VERTICAL;
    }
    if (matchAny(s, "horizontal_reversed", "h_r", "h_r", "row_reversed", "row_r", "cols_reversed", "cols_r")) {
        return ListDirection::HORIZONTAL_REVERSED;
    }
    if (matchAny(s, "vertical_reversed", "v_r", "col_reversed", "col_r", "rows_reversed", "rows_r")) {
        return ListDirection::VERTICAL_REVERSED;
    }

    MLE_W("Invalid list_direction '{}' for Container. Using VERTICAL.", str);
    return ListDirection::VERTICAL;
}

Container::ListJustify Container::strToListJustify(std::string_view str) {
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

    MLE_W("Invalid list_justify '{}' for Container. Using START.", str);
    return JustifyInt::LineMode::START;
}

Container::ListCrossAlign Container::strToListCrossAlign(std::string_view str) {
    auto s = toLower(str);
    if (matchAny(s, "start", "s", "b")) {
        return ListCrossAlign::START;
    }
    if (matchAny(s, "center", "centre", "c")) {
        return ListCrossAlign::CENTER;
    }
    if (matchAny(s, "end", "e")) {
        return ListCrossAlign::END;
    }
    if (matchAny(s, "stretch")) {
        return ListCrossAlign::STRETCH;
    }

    MLE_W("Invalid list_align_cross '{}' for Container. Using START.", str);
    return ListCrossAlign::START;
}

Container::ListWrapMode Container::strToListWrapMode(std::string_view str) {
    auto s = toLower(str);
    if (matchAny(s, "n", "no")) {
        return ListWrapMode::NO;
    }
    if (matchAny(s, "y", "yes", "wrap")) {
        return ListWrapMode::WRAP;
    }
    if (matchAny(s, "y_r", "yes_reversed", "wrap_reversed", "wrap_r")) {
        return ListWrapMode::WRAP_REVERSED;
    }

    MLE_W("Invalid list_wrap_mode '{}' for Container. Using NO.", str);
    return ListWrapMode::NO;
}

void Container::on_construct(entt::registry& registry, const entt::entity entt) {
    if (registry.any_of<SizeProvider>(entt)) {
        MLE_W("An element shouldnt have multiple SizeProvider components.");
        return;
    }
    registry.emplace<SizeProvider>(entt, [](const Entt& e, vec2u max_size) { return e.get<comp::Container>().calculateChildrenBounds(e, max_size); });
}

void Container::on_destroy(entt::registry& registry, const entt::entity entt) {
    registry.remove<SizeProvider>(entt);
};

namespace {
struct ChildBoundsCalcData {
    struct {
        comp::TargetPosition position{};
        comp::TargetSize size{};
        comp::TargetMargin margin{};
        comp::TargetBorder border{};
        f32 aspect_ratio = 0.0F;
        vec2f origin = {0.0F, 0.0F};
    } target;

    comp::SizeProvider* size_provider = nullptr;

    vec2i new_size{};
    vec2i new_position{};
    struct {
        int t, b, l, r;
    } new_margin{}, new_border{};

    explicit ChildBoundsCalcData(const Entt& e) :
        size_provider(e.tryGet<comp::SizeProvider>()) {
        MLE_T("Creating ChildBoundsUpdatingData for child: {}", e.fullName());
        MLE_T("Has size provider: {}", size_provider != nullptr);
        if (const auto* target_size = e.tryGet<comp::TargetSize>(); target_size) {
            target.size = *target_size;
            MLE_VT(target.size);
        }
        if (const auto* target_position = e.tryGet<comp::TargetPosition>(); target_position) {
            target.position = *target_position;
            MLE_VT(target.position);
        }
        if (const auto* target_margin = e.tryGet<comp::TargetMargin>(); target_margin) {
            target.margin = *target_margin;
            MLE_VT(target.margin);
        }
        if (const auto* target_border = e.tryGet<comp::TargetBorder>(); target_border) {
            target.border = *target_border;
            MLE_VT(target.border);
        }
        if (const auto* ar_comp = e.tryGet<comp::TargetAspectRatio>(); ar_comp) {
            target.aspect_ratio = ar_comp->o;
            MLE_T("Aspect Ratio: {}", target.aspect_ratio);
        }
        if (const auto* origin_comp = e.tryGet<comp::TargetOrigin>(); origin_comp) {
            target.origin = origin_comp->o;
            MLE_T("Origin: {}", target.origin);
        }
    };
};

void finishChildBounds(const Entt& centt, auto& cbcd, PaddingPx padding_px) {
    vec2i origin_lt = {padding_px.l, padding_px.t};

    comp::Bounds new_bounds;
    new_bounds.parent_px.setPos(cbcd.new_position + origin_lt);
    new_bounds.parent_px.setSize(cbcd.new_size);

    centt.emplaceOrReplace<comp::Bounds>(new_bounds);

    if (centt.has<comp::TargetBorder>()) {
        comp::Border new_border;
        new_border.t = cbcd.new_border.t;
        new_border.b = cbcd.new_border.b;
        new_border.l = cbcd.new_border.l;
        new_border.r = cbcd.new_border.r;
        new_border.color = cbcd.target.border.color;

        auto largest_size = std::max({cbcd.new_size.x, cbcd.new_size.y});
        auto roundc = [&](const TargetBound& round_tb) {
            switch (round_tb.type) {
                case TargetBound::Type::PX: {
                    return as<int>(round_tb.val);
                } break;
                case TargetBound::Type::DEFAULT:
                case TargetBound::Type::RELATIVE: {
                    return as<int>(as<f32>(largest_size) * round_tb.val);
                } break;
                case TargetBound::Type::RELATIVE_W: {
                    return as<int>(as<f32>(cbcd.new_size.x) * round_tb.val);
                } break;
                case TargetBound::Type::RELATIVE_H: {
                    return as<int>(as<f32>(cbcd.new_size.y) * round_tb.val);
                } break;
                default: {
                    // NOLINTNEXTLINE(bugprone-lambda-function-name) not a problem
                    MLE_W("Invalid border round type: {}. Treating as 0.", round_tb.type);
                    return 0;
                } break;
            }
        };
        new_border.round_lt = roundc(cbcd.target.border.round_lt);
        new_border.round_rt = roundc(cbcd.target.border.round_rt);
        new_border.round_lb = roundc(cbcd.target.border.round_lb);
        new_border.round_rb = roundc(cbcd.target.border.round_rb);

        centt.emplaceOrReplace<comp::Border>(new_border);
    }
};

void finishChildrenBounds(const Entt& e, auto& children_data, std::span<const entt::entity> to_update, PaddingPx padding_px) {
    for (auto c : to_update) {
        auto centt = Entt(e.ui(), c);
        auto& cud = children_data.at(c);

        finishChildBounds(centt, cud, padding_px);
    }
}

struct ListCalculator {
  public:
    struct ChildSizeCalcData {
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

        ChildSizeCalcData(const ChildBoundsCalcData& cbcd, bool main_is_x) {
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

  public:
    ListCalculator(Entt container_e, const Container& container, vec2i padded_size, const std::vector<entt::entity>& list_children, PaddingPx padding_px,
                   std::map<entt::entity, ChildBoundsCalcData>& cbcds) :
        container_e(container_e),
        container(container),
        list_children(list_children),
        cbcds(cbcds),
        main_is_x(container.getListDirection() == Container::ListDirection::HORIZONTAL ||
                  container.getListDirection() == Container::ListDirection::HORIZONTAL_REVERSED),
        padded_size_main(main_is_x ? padded_size.x : padded_size.y),
        padded_size_cross(main_is_x ? padded_size.y : padded_size.x),
        padded_size_main_f(as<f32>(padded_size_main)),
        padded_size_cross_f(as<f32>(padded_size_cross)),
        child_max_size(padded_size),
        child_max_size_f(child_max_size),
        child_max_size_main(main_is_x ? padded_size.x : padded_size.y),
        child_max_size_cross(calcChildMaxSizeCross(container.getListCrossMaxSize(), padded_size_cross, padded_size_cross_f,
                                                   main_is_x ? as<f32>(root_size.y) : as<f32>(root_size.x), padded_size)),
        child_max_size_main_f(as<f32>(child_max_size_main)),
        child_max_size_cross_f(as<f32>(child_max_size_cross)),
        root_size(container_e.ui().getRootSize()),
        root_size_main(main_is_x ? root_size.x : root_size.y),
        root_size_cross(main_is_x ? root_size.y : root_size.x),
        root_size_main_f(as<f32>(root_size_main)),
        root_size_cross_f(as<f32>(root_size_cross)),
        min_gap_main(calcMinGapMain(container.getListMainMinGap(), root_size_main_f, child_max_size_main_f, padded_size)),
        cross_gap(calcCrossGap(container.getListCrossGap(), root_size_cross_f, child_max_size_cross_f, padded_size)),
        main_wrap(container.getListWrapMode() != Container::ListWrapMode::NO) {
        calculateChildrenSizes();
        calculateChildrenPositions();
        finishChildrenBounds(container_e, cbcds, list_children, padding_px);
    }

    const Entt container_e;
    const Container& container;

    const std::vector<entt::entity>& list_children;
    std::map<entt::entity, ChildBoundsCalcData>& cbcds;

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
                // NOLINTNEXTLINE(bugprone-lambda-function-name) not a problem
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
                // NOLINTNEXTLINE(bugprone-lambda-function-name) not a problem
                MLE_W("Invalid cross gap type: {}. Returning 0.", tb.type);
                return 0;
            } break;
        }
    }

    bool calculateChildMarginSize(ChildSizeCalcData::CalcState& calc_state, int& ret_px, f32& ret_flex, const TargetBound& target_margin,
                                  const bool is_main) const {
        using CalcState = ChildSizeCalcData::CalcState;
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

    bool calculateChildContentSize(ChildSizeCalcData::CalcState& calc_state, int& ret_px, f32& ret_flex, const TargetBound& target_size, const bool is_main,
                                   const bool has_size_provider, const f32 aspect_ratio) const {
        using CalcState = ChildSizeCalcData::CalcState;
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

    void finishFlexCross(ChildSizeCalcData& cld) const {
        using CalcState = ChildSizeCalcData::CalcState;
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

        std::map<entt::entity, ChildSizeCalcData> children_sizes_data;

        for (auto c : list_children) {
            Entt centt{container_e.ui(), c};

            auto& cbcd = cbcds.at(c);
            auto& csd = children_sizes_data.emplace(c, ChildSizeCalcData{cbcd, main_is_x}).first->second;

            bool done = calculateChildMarginSize(csd.state.margin_main_a, csd.margin.main_a, csd.flex.margin.main_a, csd.target_main.margin_a, true);
            done &= calculateChildMarginSize(csd.state.margin_main_b, csd.margin.main_b, csd.flex.margin.main_b, csd.target_main.margin_b, true);

            if ((csd.target_cross.margin_a.type != TargetBound::Type::DEFAULT && csd.target_cross.margin_a.val != 0.0F) ||
                (csd.target_cross.margin_b.type != TargetBound::Type::DEFAULT && csd.target_cross.margin_b.val != 0.0F)) {
                MLE_W("Non-default cross margin on list child is not supported and will be ignored.");
            }

            done &= calculateChildMarginSize(csd.state.border_main_a, csd.border.main_a, csd.flex.border.main_a, csd.target_main.border_a, true);
            done &= calculateChildMarginSize(csd.state.border_main_b, csd.border.main_b, csd.flex.border.main_b, csd.target_main.border_b, true);
            done &= calculateChildMarginSize(csd.state.border_cross_a, csd.border.cross_a, csd.flex.border.cross_a, csd.target_cross.border_a, false);
            done &= calculateChildMarginSize(csd.state.border_cross_b, csd.border.cross_b, csd.flex.border.cross_b, csd.target_cross.border_b, false);

            done &= calculateChildContentSize(csd.state.main, csd.size_main, csd.flex.size.main, csd.target_main.size, true, cbcd.size_provider != nullptr,
                                              cbcd.target.aspect_ratio);

            if (container.getListCrossAlign() != Container::ListCrossAlign::STRETCH) {
                done &= calculateChildContentSize(csd.state.cross, csd.size_cross, csd.flex.size.cross, csd.target_cross.size, false,
                                                  cbcd.size_provider != nullptr, cbcd.target.aspect_ratio);
            } else {
                csd.size_cross = 0;
                csd.flex.size.cross = 1.0F;
                csd.state.cross = ChildSizeCalcData::CalcState::FLEX;
            }

            switch (container.getListCrossAlign()) {
                case Container::ListCrossAlign::CENTER: {
                    f32 flex_cross_acc = csd.flex.size.cross + csd.flex.border.cross_a + csd.flex.border.cross_b;
                    if (flex_cross_acc < 1) {
                        f32 one_minux_flex_cross_acc = 1 - flex_cross_acc;
                        csd.flex.margin.cross_a = one_minux_flex_cross_acc / 2;
                        csd.flex.margin.cross_b = csd.flex.margin.cross_a;
                        csd.state.margin_cross_a = ChildSizeCalcData::CalcState::FLEX;
                        csd.state.margin_cross_b = ChildSizeCalcData::CalcState::FLEX;
                        done = false;
                    } else {
                        csd.state.margin_cross_a = ChildSizeCalcData::CalcState::DONE;
                        csd.state.margin_cross_b = ChildSizeCalcData::CalcState::DONE;
                    }
                } break;
                case Container::ListCrossAlign::END: {
                    f32 flex_cross_acc = csd.flex.size.cross + csd.flex.border.cross_a + csd.flex.border.cross_b;
                    if (flex_cross_acc < 1) {
                        f32 one_minux_flex_cross_acc = 1 - flex_cross_acc;
                        csd.flex.margin.cross_a = one_minux_flex_cross_acc;
                        csd.flex.margin.cross_b = 0;
                        csd.state.margin_cross_a = ChildSizeCalcData::CalcState::FLEX;
                        csd.state.margin_cross_b = ChildSizeCalcData::CalcState::FLEX;
                        done = false;
                    } else {
                        csd.state.margin_cross_a = ChildSizeCalcData::CalcState::DONE;
                        csd.state.margin_cross_b = ChildSizeCalcData::CalcState::DONE;
                    }
                } break;
                default: {
                    csd.state.margin_cross_a = ChildSizeCalcData::CalcState::DONE;
                    csd.state.margin_cross_b = ChildSizeCalcData::CalcState::DONE;
                } break;
            }

            if (done) {
                csd.done = true;
                flex_acc_main += csd.flex.size.main + csd.flex.margin.main_a + csd.flex.margin.main_b + csd.flex.border.main_a + csd.flex.border.main_b;
                px_acc_main += csd.size_main + csd.margin.main_a + csd.margin.main_b + csd.border.main_a + csd.border.main_b;
                continue;
            }

            if (csd.state.main == ChildSizeCalcData::CalcState::FLEX) {
                flex_acc_main += csd.flex.size.main + csd.flex.margin.main_a + csd.flex.margin.main_b + csd.flex.border.main_a + csd.flex.border.main_b;
                px_acc_main += csd.size_main + csd.margin.main_a + csd.margin.main_b + csd.border.main_a + csd.border.main_b;
                continue;
            }

            if (csd.state.main == ChildSizeCalcData::CalcState::FIT && csd.state.cross == ChildSizeCalcData::CalcState::FIT) {
                int fit_max_size_main = child_max_size_main - csd.margin.main_a - csd.margin.main_b - csd.border.main_a - csd.border.main_b;
                int fit_max_size_cross = child_max_size_cross - csd.margin.cross_a - csd.margin.cross_b - csd.border.cross_a - csd.border.cross_b;

                if (fit_max_size_cross <= 0 || fit_max_size_main <= 0) {
                    MLE_E("Child cannot FIT in the given max size");
                    csd.valid = false;
                    continue;
                }

                vec2u p_max_size =
                    main_is_x ? vec2u{as<u32>(fit_max_size_main), as<u32>(fit_max_size_cross)} : vec2u{as<u32>(fit_max_size_cross), as<u32>(fit_max_size_main)};
                vec2u size_from_provider = cbcd.size_provider->call(centt, p_max_size);
                u32 main_size_from_provider = main_is_x ? size_from_provider.x : size_from_provider.y;
                u32 cross_size_from_provider = main_is_x ? size_from_provider.y : size_from_provider.x;

                csd.size_main = as<int>(main_size_from_provider);
                csd.state.main = ChildSizeCalcData::CalcState::DONE;

                csd.size_cross = as<int>(cross_size_from_provider);
                csd.state.cross = ChildSizeCalcData::CalcState::DONE;
            }

            if (csd.state.cross == ChildSizeCalcData::CalcState::FIT) {
                if (csd.state.main == ChildSizeCalcData::CalcState::DONE) {
                    int fit_max_size_cross = child_max_size_cross - csd.margin.cross_a - csd.margin.cross_b - csd.border.cross_a - csd.border.cross_b;
                    if (fit_max_size_cross <= 0) {
                        MLE_E("Child cannot FIT in the given max cross size");
                        csd.valid = false;
                        continue;
                    }

                    vec2u p_max_size =
                        main_is_x ? vec2u{as<u32>(csd.size_main), as<u32>(fit_max_size_cross)} : vec2u{as<u32>(fit_max_size_cross), as<u32>(csd.size_main)};
                    vec2u size_from_provider = cbcd.size_provider->call(centt, p_max_size);
                    u32 cross_size_from_provider = main_is_x ? size_from_provider.y : size_from_provider.x;

                    csd.size_cross = as<int>(cross_size_from_provider);
                    csd.state.cross = ChildSizeCalcData::CalcState::DONE;
                } else if (csd.state.main == ChildSizeCalcData::CalcState::AR) {
                    MLE_E("Cannot mix FIT and AR");
                    csd.valid = false;
                    continue;
                }
            }

            if (csd.state.cross == ChildSizeCalcData::CalcState::AR) {
                if (csd.state.main == ChildSizeCalcData::CalcState::DONE) {
                    if (main_is_x) {
                        csd.size_cross = as<int>(as<f32>(csd.size_main) / cbcd.target.aspect_ratio);
                    } else {
                        csd.size_cross = as<int>(as<f32>(csd.size_main) * cbcd.target.aspect_ratio);
                    }
                    csd.state.cross = ChildSizeCalcData::CalcState::DONE;
                } else if (csd.state.main == ChildSizeCalcData::CalcState::FIT) {
                    MLE_E("Cannot mix FIT and AR");
                    csd.valid = false;
                    continue;
                } else if (csd.state.main == ChildSizeCalcData::CalcState::AR) {
                    MLE_E("Cannot have AR on both axis.");
                    csd.valid = false;
                    continue;
                }
            }

            finishFlexCross(csd);

            if (csd.state.main == ChildSizeCalcData::CalcState::AR) {
                if (csd.state.cross == ChildSizeCalcData::CalcState::DONE) {
                    if (main_is_x) {
                        csd.size_main = as<int>(as<f32>(csd.size_cross) * cbcd.target.aspect_ratio);
                    } else {
                        csd.size_main = as<int>(as<f32>(csd.size_cross) / cbcd.target.aspect_ratio);
                    }
                    csd.state.main = ChildSizeCalcData::CalcState::DONE;
                } else {
                    MLE_E("This should be unreachable due to previous checks. Main state: AR, Cross state:{}", (int)csd.state.cross);
                    csd.valid = false;
                    continue;
                }
            }

            flex_acc_main += csd.flex.size.main + csd.flex.margin.main_a + csd.flex.margin.main_b + csd.flex.border.main_a + csd.flex.border.main_b;
            px_acc_main += csd.size_main + csd.margin.main_a + csd.margin.main_b + csd.border.main_a + csd.border.main_b;
        }

        px_acc_main += list_children.size() > 0 ? min_gap_main * (as<int>(list_children.size()) - 1) : 0;

        int remaining_px_main = as<int>(padded_size_main) - px_acc_main;
        remaining_px_main = std::max(0, remaining_px_main);
        f32 flex_share_main = (flex_acc_main > 0.0F) ? as<f32>(remaining_px_main) / std::max(1.F, flex_acc_main) : 0.0F;

        for (auto c : list_children) {
            auto& cld = children_sizes_data.at(c);
            if (!cld.valid) {
                continue;
            }
            auto& cbud = cbcds.at(c);

            if (!cld.done) {
                if (cld.state.main == ChildSizeCalcData::CalcState::FLEX) {
                    cld.size_main = as<int>(cld.flex.size.main * flex_share_main);
                    cld.state.main = ChildSizeCalcData::CalcState::DONE;
                }
                if (cld.state.margin_main_a == ChildSizeCalcData::CalcState::FLEX) {
                    cld.margin.main_a = as<int>(cld.flex.margin.main_a * flex_share_main);
                    cld.state.margin_main_a = ChildSizeCalcData::CalcState::DONE;
                }
                if (cld.state.margin_main_b == ChildSizeCalcData::CalcState::FLEX) {
                    cld.margin.main_b = as<int>(cld.flex.margin.main_b * flex_share_main);
                    cld.state.margin_main_b = ChildSizeCalcData::CalcState::DONE;
                }
                if (cld.state.border_main_a == ChildSizeCalcData::CalcState::FLEX) {
                    cld.border.main_a = as<int>(cld.flex.border.main_a * flex_share_main);
                    cld.state.border_main_a = ChildSizeCalcData::CalcState::DONE;
                }
                if (cld.state.border_main_b == ChildSizeCalcData::CalcState::FLEX) {
                    cld.border.main_b = as<int>(cld.flex.border.main_b * flex_share_main);
                    cld.state.border_main_b = ChildSizeCalcData::CalcState::DONE;
                }
                // if cross is ar
                if (cld.state.cross == ChildSizeCalcData::CalcState::AR) {
                    if (main_is_x) {
                        cld.size_cross = as<int>(as<f32>(cld.size_main) / cbud.target.aspect_ratio);
                    } else {
                        cld.size_cross = as<int>(as<f32>(cld.size_main) * cbud.target.aspect_ratio);
                    }
                    cld.state.cross = ChildSizeCalcData::CalcState::DONE;
                }
                finishFlexCross(cld);

                cld.done = cld.state.main == ChildSizeCalcData::CalcState::DONE && cld.state.cross == ChildSizeCalcData::CalcState::DONE &&
                           cld.state.margin_main_a == ChildSizeCalcData::CalcState::DONE && cld.state.margin_main_b == ChildSizeCalcData::CalcState::DONE &&
                           cld.state.margin_cross_a == ChildSizeCalcData::CalcState::DONE && cld.state.margin_cross_b == ChildSizeCalcData::CalcState::DONE &&
                           cld.state.border_main_a == ChildSizeCalcData::CalcState::DONE && cld.state.border_main_b == ChildSizeCalcData::CalcState::DONE &&
                           cld.state.border_cross_a == ChildSizeCalcData::CalcState::DONE && cld.state.border_cross_b == ChildSizeCalcData::CalcState::DONE;

                if (!cld.done) {
                    MLE_E("Could not resolve all size/margin/border for list child {}. Please fixe me!", Entt{container_e.ui(), c}.fullName());
                    MLE_E(
                        "state.main: {}, state.cross: {}, state.margin_main_a: {}, state.margin_main_b: {}, state.margin_cross_a: {}, "
                        "state.margin_cross_b: "
                        "{}, "
                        "state.border_main_a: {}, state.border_main_b: {}, state.border_cross_a: {}, state.border_cross_b: {}",
                        (int)cld.state.main, (int)cld.state.cross, (int)cld.state.margin_main_a, (int)cld.state.margin_main_b, (int)cld.state.margin_cross_a,
                        (int)cld.state.margin_cross_b, (int)cld.state.border_main_a, (int)cld.state.border_main_b, (int)cld.state.border_cross_a,
                        (int)cld.state.border_cross_b);
                    cld.valid = false;
                    continue;
                }
            }

            MLE_ASSERT_LOG(cld.size_main > 0 && cld.size_cross > 0, "Negative or zero size calculated for list child {}. main: {}, cross: {}",
                           Entt{container_e.ui(), c}.fullName(), cld.size_main, cld.size_cross);

            if (main_is_x) {
                cbud.new_size.x = cld.size_main;
                cbud.new_size.y = cld.size_cross;
                cbud.new_margin.l = cld.margin.main_a;
                cbud.new_margin.r = cld.margin.main_b;
                cbud.new_margin.t = cld.margin.cross_a;
                cbud.new_margin.b = cld.margin.cross_b;
                cbud.new_border.l = cld.border.main_a;
                cbud.new_border.r = cld.border.main_b;
                cbud.new_border.t = cld.border.cross_a;
                cbud.new_border.b = cld.border.cross_b;
            } else {
                cbud.new_size.y = cld.size_main;
                cbud.new_size.x = cld.size_cross;
                cbud.new_margin.t = cld.margin.main_a;
                cbud.new_margin.b = cld.margin.main_b;
                cbud.new_margin.l = cld.margin.cross_a;
                cbud.new_margin.r = cld.margin.cross_b;
                cbud.new_border.t = cld.border.main_a;
                cbud.new_border.b = cld.border.main_b;
                cbud.new_border.l = cld.border.cross_a;
                cbud.new_border.r = cld.border.cross_b;
            }
        }
    }

    void calculateChildrenPositions() const {
        std::vector<int> children_main_sizes;
        for (auto c : list_children) {
            auto& cbcd = cbcds.at(c);
            auto total_margin_border_main = main_is_x ? (cbcd.new_margin.l + cbcd.new_margin.r + cbcd.new_border.l + cbcd.new_border.r)
                                                      : (cbcd.new_margin.t + cbcd.new_margin.b + cbcd.new_border.t + cbcd.new_border.b);
            auto total_size_main = main_is_x ? cbcd.new_size.x : cbcd.new_size.y;
            total_size_main += total_margin_border_main;
            children_main_sizes.push_back(total_size_main);
        }

        const auto main_reversed = container.getListDirectionIsReversed();
        const auto cross_reversed = container.getListWrapMode() == Container::ListWrapMode::WRAP_REVERSED;

        const auto container_origin_main = main_reversed ? padded_size_main : 0;
        const auto container_origin_cross = cross_reversed ? padded_size_cross : 0;

        auto current_pos_cross = container_origin_cross;
        if (cross_reversed) {
            current_pos_cross -= child_max_size_cross;
        }

        switch (container.getListWrapMode()) {
            case Container::ListWrapMode::NO: {
                auto line = JustifyInt::justifyUntilOverflow(children_main_sizes, min_gap_main, container.getListJustify(), container.getListJustify(),
                                                             child_max_size_main);
                if (container.isScrollable() && line.size() != list_children.size()) {
                    line = JustifyInt::justifyLineStart(children_main_sizes, min_gap_main);
                }
                for (usize i = 0; i < line.size(); i++) {
                    auto centt = Entt{container_e.ui(), list_children[i]};
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
            case Container::ListWrapMode::WRAP_REVERSED:
            case Container::ListWrapMode::WRAP: {
                auto lines =
                    JustifyInt::wrap(children_main_sizes, min_gap_main, container.getListJustify(), container.getListJustifyLast(), child_max_size_main);
                usize line_index = 0;
                usize child_index_in_line = 0;
                for (usize i = 0; i < list_children.size(); i++) {
                    auto centt = Entt{container_e.ui(), list_children[i]};
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
                MLE_UNREACHABLE_LOG("Invalid wrap mode: {}", (int)container.getListWrapMode());
            } break;
        }
    }
};

struct FreeCalculator {
    // NOLINTNEXTLINE(readability-function-cognitive-complexity) Not that complex
    FreeCalculator(Entt container_e, [[maybe_unused]] const Container& container, vec2i padded_size, const std::vector<entt::entity>& free_children,
                   PaddingPx padding_px, std::map<entt::entity, ChildBoundsCalcData>& cbcds) {
        auto sorted_by_dependencies = sortByDependency(container_e.ui(), free_children);

        const auto root_size = container_e.ui().getRootSize();
        const auto root_size_f = vec2f{as<f32>(root_size.x), as<f32>(root_size.y)};

        std::unordered_map<entt::entity, Recti> children_full_rect;
        children_full_rect.reserve(free_children.size());

        for (auto c : sorted_by_dependencies) {
            Entt centt{container_e.ui(), c};
            MLE_VC(centt.name());
            auto& cbcd = cbcds.at(c);

            switch (cbcd.target.position.x.type) {
                case TargetBound::Type::PX: {
                    cbcd.new_position.x = as<int>(cbcd.target.position.x.val);
                } break;
                case TargetBound::Type::ROOT: {
                    cbcd.new_position.x = as<int>(root_size_f.x * cbcd.target.position.x.val);
                } break;
                case TargetBound::Type::DEFAULT:
                case TargetBound::Type::RELATIVE_W:
                case TargetBound::Type::RELATIVE: {
                    cbcd.new_position.x = as<int>(as<f32>(padded_size.x) * cbcd.target.position.x.val);
                } break;
                case TargetBound::Type::RELATIVE_H: {
                    cbcd.new_position.x = as<int>(as<f32>(padded_size.y) * cbcd.target.position.x.val);
                } break;
                default: {
                    MLE_E("Invalid target position x type: {}.", cbcd.target.position.x.type);
                } break;
            }

            if (cbcd.target.position.xdep.e != entt::null) {
                if (!centt.getRelationship().isChildOf(container_e.e())) {
                    MLE_W("Child {} is not a child of the container. Ignoring x dependency.", centt.name());
                } else {
                    const auto dep_entt = Entt{container_e.ui(), cbcd.target.position.xdep.e};
                    const auto tb = cbcd.target.position.xdep.dep_tb;
                    Recti dep_rect;
                    auto dep_it = children_full_rect.find(dep_entt.e());
                    if (dep_it != children_full_rect.end()) {
                        dep_rect = dep_it->second;
                    } else {
                        auto& dep_bounds = dep_entt.get<comp::Bounds>();
                        dep_rect = dep_bounds.parent_px;
                    }

                    cbcd.new_position.x += dep_rect.left();
                    switch (tb.type) {
                        case TargetBound::Type::PX: {
                            cbcd.new_position.x += as<int>(tb.val) + dep_rect.width();
                        } break;
                        case TargetBound::Type::DEFAULT:
                        case TargetBound::Type::RELATIVE:
                        case TargetBound::Type::RELATIVE_W: {
                            cbcd.new_position.x += as<int>(as<f32>(dep_rect.width()) * tb.val);
                        } break;
                        default: {
                            MLE_W("Invalid x dependency target bound type: {}. Ignoring extra.", tb.type);
                        } break;
                    }
                }
            }

            switch (cbcd.target.position.y.type) {
                case TargetBound::Type::PX: {
                    cbcd.new_position.y = as<int>(cbcd.target.position.y.val);
                } break;
                case TargetBound::Type::ROOT: {
                    cbcd.new_position.y = as<int>(root_size_f.y * cbcd.target.position.y.val);
                } break;
                case TargetBound::Type::DEFAULT:
                case TargetBound::Type::RELATIVE_W:
                case TargetBound::Type::RELATIVE: {
                    cbcd.new_position.y = as<int>(as<f32>(padded_size.y) * cbcd.target.position.y.val);
                } break;
                case TargetBound::Type::RELATIVE_H: {
                    cbcd.new_position.y = as<int>(as<f32>(padded_size.x) * cbcd.target.position.y.val);
                } break;
                default: {
                    MLE_W("Invalid target position y type: {}.", cbcd.target.position.y.type);
                } break;
            }

            if (cbcd.target.position.ydep.e != entt::null) {
                if (!centt.getRelationship().isChildOf(container_e.e())) {
                    MLE_W("Child {} is not a child of the container. Ignoring y dependency.", centt.name());
                } else {
                    const auto dep_entt = Entt{container_e.ui(), cbcd.target.position.ydep.e};
                    const auto tb = cbcd.target.position.ydep.dep_tb;
                    Recti dep_rect;
                    auto dep_it = children_full_rect.find(dep_entt.e());
                    if (dep_it != children_full_rect.end()) {
                        dep_rect = dep_it->second;
                    } else {
                        auto& dep_bounds = dep_entt.get<comp::Bounds>();
                        dep_rect = dep_bounds.parent_px;
                    }
                    cbcd.new_position.y += dep_rect.top();
                    switch (tb.type) {
                        case TargetBound::Type::PX: {
                            cbcd.new_position.y += as<int>(tb.val) + dep_rect.height();
                        } break;
                        case TargetBound::Type::DEFAULT:
                        case TargetBound::Type::RELATIVE:
                        case TargetBound::Type::RELATIVE_W: {
                            cbcd.new_position.y += as<int>(as<f32>(dep_rect.height()) * tb.val);
                        } break;
                        default: {
                            MLE_W("Invalid y dependency target bound type: {}. Ignoring extra.", tb.type);
                        } break;
                    }
                }
            }

            bool x_is_ar = false;
            bool x_is_fit = false;
            bool y_is_fit = false;

            switch (cbcd.target.size.x.type) {
                case TargetBound::Type::PX: {
                    cbcd.new_size.x = as<int>(cbcd.target.size.x.val);
                } break;
                case TargetBound::Type::ROOT: {
                    cbcd.new_size.x = as<int>(root_size_f.x * cbcd.target.size.x.val);
                } break;
                case TargetBound::Type::RELATIVE_W:
                case TargetBound::Type::RELATIVE: {
                    cbcd.new_size.x = as<int>(as<f32>(padded_size.x) * cbcd.target.size.x.val);
                }; break;
                case TargetBound::Type::RELATIVE_H: {
                    cbcd.new_size.x = as<int>(as<f32>(padded_size.y) * cbcd.target.size.x.val);
                } break;
                case TargetBound::Type::FLEX_SHARE: {
                    auto remaining_px = padded_size.x - cbcd.new_position.x;
                    cbcd.new_size.x = as<int>(cbcd.target.size.x.val * as<f32>(remaining_px));
                } break;
                case TargetBound::Type::FIT: {
                    if (cbcd.size_provider == nullptr) {
                        MLE_E("FIT target size x requires a size provider. Entity:{}", centt.fullName());
                        continue;
                    }
                    x_is_fit = true;
                } break;
                case TargetBound::Type::DEFAULT: {
                    if (cbcd.target.size.x.val != 0) {
                        cbcd.new_size.x = as<int>(as<f32>(padded_size.x) * cbcd.target.size.x.val);
                    } else if (cbcd.target.size.xdep.e == entt::null) {
                        if (cbcd.target.aspect_ratio > 0) {
                            x_is_ar = true;
                        } else if (cbcd.size_provider != nullptr) {
                            x_is_fit = true;
                        } else {
                            auto remaining_px = padded_size.x - cbcd.new_position.x;
                            cbcd.new_size.x = as<int>(as<f32>(remaining_px));
                        }
                    }
                } break;
                default: {
                    MLE_E("Weird enum value for target size x type: {}.", (int)cbcd.target.size.x.type);
                    continue;
                } break;
            }

            if (cbcd.target.size.xdep.e != entt::null) {
                if (!centt.getRelationship().isChildOf(container_e.e())) {
                    MLE_W("Child {} is not a child of the container. Ignoring x size dependency.", centt.name());
                } else {
                    const auto dep_entt = Entt{container_e.ui(), cbcd.target.size.xdep.e};
                    const auto tb = cbcd.target.size.xdep.dep_tb;
                    Recti dep_rect;
                    auto dep_it = children_full_rect.find(dep_entt.e());
                    if (dep_it != children_full_rect.end()) {
                        dep_rect = dep_it->second;
                    } else {
                        auto& dep_bounds = dep_entt.get<comp::Bounds>();
                        dep_rect = dep_bounds.parent_px;
                    }
                    switch (tb.type) {
                        case TargetBound::Type::PX: {
                            cbcd.new_size.x += as<int>(tb.val) + dep_rect.width();
                        } break;
                        case TargetBound::Type::DEFAULT:
                        case TargetBound::Type::RELATIVE:
                        case TargetBound::Type::RELATIVE_W: {
                            cbcd.new_size.x += as<int>(as<f32>(dep_rect.width()) * tb.val);
                        } break;
                        default: {
                            MLE_W("Invalid x size dependency target bound type: {}. Ignoring extra.", tb.type);
                        } break;
                    }
                }
            }

            switch (cbcd.target.size.y.type) {
                case TargetBound::Type::PX: {
                    cbcd.new_size.y = as<int>(cbcd.target.size.y.val);
                } break;
                case TargetBound::Type::ROOT: {
                    cbcd.new_size.y = as<int>(root_size_f.y * cbcd.target.size.y.val);
                }
                case TargetBound::Type::RELATIVE_H:
                case TargetBound::Type::RELATIVE: {
                    cbcd.new_size.y = as<int>(as<f32>(padded_size.y) * cbcd.target.size.y.val);
                }; break;
                case TargetBound::Type::RELATIVE_W: {
                    cbcd.new_size.y = as<int>(as<f32>(padded_size.x) * cbcd.target.size.y.val);
                } break;
                case TargetBound::Type::FLEX_SHARE: {
                    auto remaining_px = padded_size.y - cbcd.new_position.y;
                    cbcd.new_size.y = as<int>(cbcd.target.size.y.val * as<f32>(remaining_px));
                } break;
                case TargetBound::Type::FIT: {
                    if (x_is_ar) {
                        MLE_E("Cannot mix FIT and AR.");
                        continue;
                    }
                    if (cbcd.size_provider == nullptr) {
                        MLE_E("FIT target size y requires a size provider. Entity name:{}", centt.fullName());
                        continue;
                    }
                    y_is_fit = true;
                } break;
                case TargetBound::Type::DEFAULT: {
                    if (cbcd.target.size.y.val != 0) {
                        cbcd.new_size.y = as<int>(as<f32>(padded_size.y) * cbcd.target.size.y.val);
                    } else if (cbcd.target.size.ydep.e == entt::null) {
                        if (cbcd.size_provider != nullptr && !x_is_ar) {
                            y_is_fit = true;
                        } else {
                            auto remaining_px = padded_size.y - cbcd.new_position.y;
                            cbcd.new_size.y = as<int>(as<f32>(remaining_px));
                        }
                    }
                } break;
                default: {
                    MLE_E("Weird enum value for target size y type: {}.", (int)cbcd.target.size.y.type);
                    continue;
                } break;
            }

            if (cbcd.target.size.ydep.e != entt::null) {
                if (!centt.getRelationship().isChildOf(container_e.e())) {
                    MLE_W("Child {} is not a child of the container. Ignoring y size dependency.", centt.name());
                } else {
                    const auto dep_entt = Entt{container_e.ui(), cbcd.target.size.ydep.e};
                    const auto tb = cbcd.target.size.ydep.dep_tb;
                    Recti dep_rect;
                    auto dep_it = children_full_rect.find(dep_entt.e());
                    if (dep_it != children_full_rect.end()) {
                        dep_rect = dep_it->second;
                    } else {
                        auto& dep_bounds = dep_entt.get<comp::Bounds>();
                        dep_rect = dep_bounds.parent_px;
                    }
                    switch (tb.type) {
                        case TargetBound::Type::PX: {
                            cbcd.new_size.y += as<int>(tb.val) + dep_rect.height();
                        } break;
                        case TargetBound::Type::DEFAULT:
                        case TargetBound::Type::RELATIVE:
                        case TargetBound::Type::RELATIVE_W: {
                            cbcd.new_size.y += as<int>(as<f32>(dep_rect.height()) * tb.val);
                        } break;
                        default: {
                            MLE_W("Invalid y size dependency target bound type: {}. Ignoring extra.", tb.type);
                        } break;
                    }
                }
            }

            if (x_is_ar) {
                cbcd.new_size.x = as<int>(as<f32>(cbcd.new_size.y) * cbcd.target.aspect_ratio);
            }

            if (x_is_fit || y_is_fit) {
                vec2u fit_max_size_px = {cbcd.new_size.x, cbcd.new_size.y};

                if (fit_max_size_px.x <= 0) {
                    fit_max_size_px.x = padded_size.x - cbcd.new_position.x;
                }
                if (fit_max_size_px.y <= 0) {
                    fit_max_size_px.y = padded_size.y - cbcd.new_position.y;
                }

                vec2u size_from_provider = cbcd.size_provider->call(centt, fit_max_size_px);

                if (x_is_fit) {
                    cbcd.new_size.x = as<int>(size_from_provider.x);
                }
                if (y_is_fit) {
                    cbcd.new_size.y = as<int>(size_from_provider.y);
                }
            }
            //////////////// MARGIN & BORDER CALCULATION ////////////////

            auto calc_margin_border = [&](TargetBound tb, bool is_x) -> int {
                switch (tb.type) {
                    case TargetBound::Type::PX: {
                        return as<int>(tb.val);
                    } break;
                    case TargetBound::Type::ROOT: {
                        return as<int>((is_x ? root_size_f.x : root_size_f.y) * tb.val);
                    } break;
                    case TargetBound::Type::DEFAULT:
                    case TargetBound::Type::RELATIVE: {
                        if (is_x) {
                            return as<int>(as<f32>(padded_size.x) * tb.val);
                        }
                        return as<int>(as<f32>(padded_size.y) * tb.val);
                    } break;
                    case TargetBound::Type::RELATIVE_W: {
                        return as<int>(as<f32>(padded_size.x) * tb.val);
                    } break;
                    case TargetBound::Type::RELATIVE_H: {
                        return as<int>(as<f32>(padded_size.y) * tb.val);
                    } break;
                    default: {
                        MLE_W("Invalid target margin/border type: {}.", (int)tb.type);
                        return 0;
                    } break;
                }
            };

            cbcd.new_margin.t = calc_margin_border(cbcd.target.margin.t, false);
            cbcd.new_margin.b = calc_margin_border(cbcd.target.margin.b, false);
            cbcd.new_margin.l = calc_margin_border(cbcd.target.margin.l, true);
            cbcd.new_margin.r = calc_margin_border(cbcd.target.margin.r, true);
            cbcd.new_border.t = calc_margin_border(cbcd.target.border.t, false);
            cbcd.new_border.b = calc_margin_border(cbcd.target.border.b, false);
            cbcd.new_border.l = calc_margin_border(cbcd.target.border.l, true);
            cbcd.new_border.r = calc_margin_border(cbcd.target.border.r, true);

            auto& this_margin = children_full_rect[c];
            this_margin.setWidth(cbcd.new_size.x + cbcd.new_margin.l + cbcd.new_margin.r + cbcd.new_border.l + cbcd.new_border.r);
            this_margin.setHeight(cbcd.new_size.y + cbcd.new_margin.t + cbcd.new_margin.b + cbcd.new_border.t + cbcd.new_border.b);

            vec2f margin_origin = vec2f(this_margin.size()) * cbcd.target.origin;
            this_margin.setPosX(cbcd.new_position.x - as<int>(margin_origin.x));
            this_margin.setPosY(cbcd.new_position.y - as<int>(margin_origin.y));

            cbcd.new_position.x = this_margin.left() + cbcd.new_margin.l + cbcd.new_border.l;

            cbcd.new_position.y = this_margin.top() + cbcd.new_margin.t + cbcd.new_border.t;

            finishChildBounds(centt, cbcd, padding_px);
        }
    }

    static std::vector<entt::entity> sortByDependency(UI& ui, std::span<const entt::entity> span) {
        enum class Mark : u8 { UNSEEN = 0, VISITING = 1, DONE = 2 };
        std::map<entt::entity, Mark> mark;

        std::vector<entt::entity> out;
        out.reserve(span.size());

        auto find_it = [](auto span, entt::entity e) { return std::find_if(span.begin(), span.end(), [e](entt::entity o) { return o == e; }); };
        // NOLINTNEXTLINE(misc-no-recursion) yeah, cool recursion
        auto dfs = [&](auto&& self, entt::entity e) {
            const auto mark_it = mark.find(e);
            if (mark_it != mark.end()) {
                if (mark_it->second == Mark::DONE) {
                    return true;
                }
                if (mark_it->second == Mark::VISITING) {
                    // NOLINTNEXTLINE(bugprone-lambda-function-name) just a log
                    MLE_ASSERT_LOG(false, "Cycle detected while sorting UI deps at entity {}", e);
                    return false;
                }
            }

            mark[e] = Mark::VISITING;

            Entt ee(ui, e);

            std::array<entt::entity, 4> deps{};
            usize n = 0;
            auto push_unique = [&](entt::entity d) {
                if (d == entt::null) {
                    return;
                }
                for (usize i = 0; i < n; ++i) {
                    if (deps.at(i) == d) {
                        return;
                    }
                }
                if (n < deps.size()) {
                    deps.at(n++) = d;
                }
            };

            if (const auto* pos = ee.tryGet<comp::TargetPosition>()) {
                push_unique(pos->xdep.e);
                push_unique(pos->ydep.e);
            }
            if (const auto* size = ee.tryGet<comp::TargetSize>()) {
                push_unique(size->xdep.e);
                push_unique(size->ydep.e);
            }

            for (usize i = 0; i < n; ++i) {
                const entt::entity d = deps.at(i);
                if (find_it(span, d) == span.end()) {
                    continue;  // skip deps outside of the span, maybe is a list child
                }
                if (!self(self, d)) {
                    return false;
                }
            }

            mark[e] = Mark::DONE;
            out.push_back(e);
            return true;
        };

        for (const auto& e : span) {
            if (!mark.contains(e)) {
                if (!dfs(dfs, e)) {
                    MLE_E("Cycle detected while sorting UI deps. Returning empty list.");
                    return {};
                }
            }
        }

        MLE_ASSERT_LOG(out.size() == span.size(), "Sorted UI deps size mismatch. Check this. {}x{}", out.size(), span.size());

        return out;
    }
};

std::pair<std::vector<entt::entity>, std::vector<entt::entity>> separateFreeListChildren(UI& ui, std::span<entt::entity> span) {
    std::vector<entt::entity> free_children;
    std::vector<entt::entity> list_children;

    for (const auto& e : span) {
        Entt ee{ui, e};
        if (!ee.has<comp::TargetPosition>()) {
            list_children.push_back(e);
        } else {
            free_children.push_back(e);
        }
    }

    return {free_children, list_children};
}

vec2u accumulateChildrenExtent(const std::map<entt::entity, ChildBoundsCalcData>& cbcds, const PaddingPx& padding) {
    if (cbcds.empty()) {
        return vec2u{0, 0};
    }

    vec2i min_pos{std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
    vec2i max_pos{std::numeric_limits<int>::min(), std::numeric_limits<int>::min()};

    for (const auto& [_, cud] : cbcds) {
        vec2i child_min = cud.new_position - vec2i{cud.new_margin.l + cud.new_border.l, cud.new_margin.t + cud.new_border.t};
        vec2i child_max = cud.new_position + vec2i{cud.new_size.x + cud.new_margin.r + cud.new_border.r, cud.new_size.y + cud.new_margin.b + cud.new_border.b};

        min_pos.x = std::min(min_pos.x, child_min.x);
        min_pos.y = std::min(min_pos.y, child_min.y);
        max_pos.x = std::max(max_pos.x, child_max.x);
        max_pos.y = std::max(max_pos.y, child_max.y);
    }

    if (min_pos.x == std::numeric_limits<int>::max() || min_pos.y == std::numeric_limits<int>::max()) {
        return vec2u{0, 0};
    }

    vec2i total_size = (max_pos - min_pos) + vec2i{padding.l + padding.r, padding.t + padding.b};
    return vec2u{as<u32>(total_size.x), as<u32>(total_size.y)};
}
}  // namespace

[[nodiscard]] vec2u Container::calculateChildrenBounds(const Entt& e, vec2u max_size) const {
    auto& self_rel = e.getRelationship();
    auto children = self_rel.getChildren(e);

    const auto* padding_comp = e.tryGet<comp::TargetPadding>();
    PaddingPx padding_result{};
    if (padding_comp) {
        padding_result = padding_comp->calc(e.ui(), max_size);
    }
    vec2i padded_max_size = padding_result.removeFrom(max_size);

    std::map<entt::entity, ChildBoundsCalcData> cbcds;
    for (const auto& c : children) {
        Entt centt{e.ui(), c};
        cbcds.emplace(c, centt);
    }

    switch (type_) {
        case Type::HYBRID: {
            auto [free_children, list_children] = separateFreeListChildren(e.ui(), children);
            ListCalculator list_calculator{e, *this, padded_max_size, list_children, padding_result, cbcds};
            FreeCalculator free_calculator{e, *this, padded_max_size, free_children, padding_result, cbcds};
        } break;
        case Type::LIST: {
            ListCalculator list_calculator{e, *this, padded_max_size, children, padding_result, cbcds};
        } break;
        case Type::FREE: {
            FreeCalculator free_calculator{e, *this, padded_max_size, children, padding_result, cbcds};
        } break;
    }

    for (auto c : children) {
        Entt centt{e.ui(), c};
        auto& cbcd = cbcds.at(c);
        if (cbcd.size_provider) {
            cbcd.size_provider->call(centt, cbcd.new_size);
            cbcd.size_provider->reset();
        }
    }

    return accumulateChildrenExtent(cbcds, padding_result);
}
}  // namespace mle::ui::comp
