#include "Container.h"

#include <string>

#include "mle/lua/Utils.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Bounds.h"
#include "mle/utils/ECS.h"
#include "mle/utils/ID.h"
#include "mle/utils/Justify.h"
#include "mle/utils/String.h"

namespace mle::ui::comp {
namespace {
auto justifyStringToEnum(std::string_view str) {
    using JustifyMode = JustifyInt::LineMode;
    if (matchAny(str, "start", "flex_start")) {
        return JustifyMode::START;
    }
    if (matchAny(str, "center", "centre", "c")) {
        return JustifyMode::CENTER;
    }
    if (matchAny(str, "end", "flex_end")) {
        return JustifyMode::END;
    }
    if (matchAny(str, "space_between", "space-between")) {
        return JustifyMode::SPACE_BETWEEN;
    }
    if (matchAny(str, "space_around", "space-around")) {
        return JustifyMode::SPACE_AROUND;
    }
    if (matchAny(str, "space_evenly", "space-evenly")) {
        return JustifyMode::SPACE_EVENLY;
    }
    MLE_W("Invalid justify mode string '{}', using START.", str);
    return JustifyMode::START;
}
}  // namespace

void Container::set(const Entt& e, const sol::table& table) {
    if (const auto c_r = lua::tryGetAnyKey(table, "c", "children"); lua::valid<sol::table>(c_r)) {
        addMany(e, lua::as<sol::table>(c_r));
    }
    if (const auto child_r = table["child"]; lua::valid<sol::table>(child_r)) {
        addChild(e, child_r);
    }
    if (const auto type_r = table["type"]; lua::valid<std::string>(type_r)) {
        const auto type_str = toLower(lua::as<std::string>(type_r));
        if (matchAny(type_str, "hybrid")) {
            type = Type::HYBRID;
        } else if (matchAny(type_str, "list")) {
            type = Type::LIST;
        } else if (matchAny(type_str, "free")) {
            type = Type::FREE;
        } else {
            MLE_W("Invalid container type '{}' for Container at entity {}. Using HYBRID.", type_str, e.name());
            type = Type::HYBRID;
        }
    }
    if (const auto min_gap_main_r = table["min_gap"]; lua::valid<sol::object>(min_gap_main_r)) {
        min_gap_main.set(min_gap_main_r);
    }
    if (const auto list_direction_r = lua::tryGetAnyKey(table, "list_dir", "list_direction", "direction", "dir"); lua::valid<std::string>(list_direction_r)) {
        const auto dir_str = toLower(lua::as<std::string>(list_direction_r));
        if (matchAny(dir_str, "horizontal", "h", "row", "cols")) {
            list_direction = ListDirection::HORIZONTAL;
        } else if (matchAny(dir_str, "vertical", "v", "col", "rows")) {
            list_direction = ListDirection::VERTICAL;
        } else if (matchAny(dir_str, "horizontal_reversed", "h_reversed", "h_rev", "row_reversed", "row_rev", "cols_reversed", "cols_rev")) {
            list_direction = ListDirection::HORIZONTAL_REVERSED;
        } else if (matchAny(dir_str, "vertical_reversed", "v_reversed", "v_rev", "col_reversed", "col_rev", "rows_reversed", "rows_rev")) {
            list_direction = ListDirection::VERTICAL_REVERSED;
        } else {
            MLE_W("Invalid list_direction '{}' for Container at entity {}. Using VERTICAL.", dir_str, e.name());
            list_direction = ListDirection::VERTICAL;
        }
    }
    if (const auto list_align_main_r = lua::tryGetAnyKey(table, "list_align_main", "align_main", "main_align", "justify");
        lua::valid<std::string>(list_align_main_r)) {
        const auto align_str = toLower(lua::as<std::string>(list_align_main_r));
        list_align_main = justifyStringToEnum(align_str);
    }
    if (const auto list_align_cross_r = lua::tryGetAnyKey(table, "list_align_cross", "align_cross", "cross_align");
        lua::valid<std::string>(list_align_cross_r)) {
        const auto align_str = toLower(lua::as<std::string>(list_align_cross_r));
        if (matchAny(align_str, "start", "flex_start")) {
            list_align_cross = AlignCross::START;
        } else if (matchAny(align_str, "center", "centre", "c")) {
            list_align_cross = AlignCross::CENTER;
        } else if (matchAny(align_str, "end", "flex_end")) {
            list_align_cross = AlignCross::END;
        } else if (matchAny(align_str, "stretch")) {
            list_align_cross = AlignCross::STRETCH;
        } else {
            MLE_W("Invalid list_align_cross '{}' for Container at entity {}. Using START.", align_str, e.name());
            list_align_cross = AlignCross::START;
        }
    }
}

std::tuple<sol::table, entt::entity, std::string, usize> Container::createChildEnttHnd(const Entt& e, const sol::table& table, std::string name, usize pos) {
    entt::entity child_e = e.ui().getLuaElementOps().createElement(e.e());

    if (name.empty()) {
        if (const auto name_r = table["name"]; lua::valid<std::string>(name_r)) {
            name = lua::as<std::string>(name_r);
        } else {
            name = "__" + std::to_string(genID());
        }
    }

    if (pos == max<usize>()) {
        if (const auto pos_r = table["pos"]; lua::valid<usize>(pos_r)) {
            pos = lua::as<usize>(pos_r);
        }
    }

    o.add(name, child_e, pos);

    if (const auto comp_r = table["c"]; lua::valid<sol::table>(comp_r)) {
        return std::make_tuple(lua::as<sol::table>(comp_r), child_e, name, pos);
    }

    return std::make_tuple(table, child_e, name, pos);
}

void Container::addChild(const Entt& e, const sol::table& table, const std::string& name, usize pos) {
    auto [comp_table, ce_hnd, final_name, final_pos] = createChildEnttHnd(e, table, name, pos);
    Entt centt{e.ui(), ce_hnd};
    centt.applyTable(element_base.valid() ? e.ui().getLua().mergeTablesNew(element_base, comp_table) : comp_table);
}

void Container::addMany(const Entt& e, const sol::table& table) {
    std::vector<std::tuple<sol::table, entt::entity, std::string, usize>> new_children_info;
    for (const auto& [key, value] : table) {
        auto child_table_r = lua::tryAs<sol::table>(value);
        if (!child_table_r) {
            MLE_W("Invalid child to add to Container at entity {}. Expected table.", e.name());
            continue;
        }

        std::string name;
        if (key.is<std::string>()) {
            name = key.as<std::string>();
        }

        new_children_info.push_back(createChildEnttHnd(e, *child_table_r, name, max<usize>()));
    }

    for (const auto& [comp_table, ce_hnd, final_name, final_pos] : new_children_info) {
        Entt centt{e.ui(), ce_hnd};
        centt.applyTable(element_base.valid() ? e.ui().getLua().mergeTablesNew(element_base, comp_table) : comp_table);
    }
}

void Container::apply(const Entt& e, const sol::object& obj) {
    auto table_r = lua::tryAs<sol::table>(obj);
    if (!table_r) {
        MLE_W("Invalid object to apply Container component at entity {}. Expected table.", e.name());
        return;
    }
    e.patchOrEmplace<Container>([&](Container& c) { c.set(e, *table_r); });
}

void Container::applyAddChild(const Entt& e, const sol::object& obj) {
    e.patchOrEmplace<Container>([&](Container& c) {
        auto table_r = lua::tryAs<sol::table>(obj);
        if (!table_r) {
            MLE_W("Invalid object to add child to Container at entity {}. Expected table.", e.name());
            return;
        }
        c.addChild(e, *table_r);
    });
}

void Container::applyAddChildren(const Entt& e, const sol::object& obj) {
    e.patchOrEmplace<Container>([&](Container& c) {
        auto table_r = lua::tryAs<sol::table>(obj);
        if (!table_r) {
            MLE_W("Invalid object to add children to Container at entity {}. Expected table.", e.name());
            return;
        }
        c.addMany(e, *table_r);
    });
}

namespace {
std::vector<entt::entity> sortChildrenByDependency(UI& ui, std::span<const entt::entity> span) {
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

std::pair<std::vector<entt::entity>, std::vector<entt::entity>> separateFreeListChildren(UI& ui, std::span<const EntityStorage::Entry> span) {
    std::vector<entt::entity> free_children;
    std::vector<entt::entity> list_children;

    for (const auto& e : span) {
        Entt ee{ui, e.e};
        if (!ee.has<comp::TargetPosition>()) {
            list_children.push_back(e.e);
        } else {
            free_children.push_back(e.e);
        }
    }

    return {free_children, list_children};
}

struct ChildUpdatingData {
    comp::TargetPosition target_position{};
    comp::TargetSize target_size{};
    comp::TargetMargin target_margin{};
    comp::TargetBorder target_border{};
    comp::SizeProvider* size_provider = nullptr;
    f32 target_aspect_ratio = 0.0F;
    vec2f target_origin = {0.0F, 0.0F};

    vec2i new_size{};
    vec2i new_position{};
    struct {
        int t, b, l, r;
    } new_margin{}, new_border{};

    struct ListSizeData {
        enum class CalcState : u8 { UNSEEN, DONE, FLEX, FIT, AR };
        bool valid = true;
        bool done = false;

        struct {
            int main_a{}, main_b{}, cross_a{}, cross_b{};
        } margin_px{}, border_px{};

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

        int size_main_px = 0;
        int size_cross_px = 0;
        int pos_main_px = 0;
        int pos_cross_px = 0;

        struct {
            CalcState main, cross, margin_main_a, margin_main_b, margin_cross_a, margin_cross_b, border_main_a, border_main_b, border_cross_a, border_cross_b;
        } state{};
    };

    explicit ChildUpdatingData(const Entt& e) :
        size_provider(e.tryGet<comp::SizeProvider>()) {
        MLE_T("Creating ChildUpdatingData for child: {}", e.name());
        MLE_T("Has size provider: {}", size_provider != nullptr);
        if (const auto* target_size = e.tryGet<comp::TargetSize>(); target_size) {
            this->target_size = *target_size;
            MLE_VT(this->target_size);
        }
        if (const auto* target_position = e.tryGet<comp::TargetPosition>(); target_position) {
            this->target_position = *target_position;
            MLE_VT(this->target_position);
        }
        if (const auto* target_margin = e.tryGet<comp::TargetMargin>(); target_margin) {
            this->target_margin = *target_margin;
            MLE_VT(this->target_margin);
        }
        if (const auto* target_border = e.tryGet<comp::TargetBorder>(); target_border) {
            this->target_border = *target_border;
            MLE_VT(this->target_border);
        }
        if (const auto* aspect_ratio = e.tryGet<comp::TargetAspectRatio>(); aspect_ratio) {
            target_aspect_ratio = aspect_ratio->o;
            MLE_VT(target_aspect_ratio);
        }
        if (const auto* origin = e.tryGet<comp::TargetOrigin>(); origin) {
            target_origin = origin->o;
            MLE_VT(target_origin);
        }
    };

    [[nodiscard]] ListSizeData gatterListData(bool main_is_x) {
        ListSizeData list_data{};
        if (main_is_x) {
            list_data.target_main = {
                .size = target_size.x, .margin_a = target_margin.l, .margin_b = target_margin.r, .border_a = target_border.l, .border_b = target_border.r};
            list_data.target_cross = {
                .size = target_size.y, .margin_a = target_margin.t, .margin_b = target_margin.b, .border_a = target_border.t, .border_b = target_border.b};
        } else {
            list_data.target_main = {
                .size = target_size.y, .margin_a = target_margin.t, .margin_b = target_margin.b, .border_a = target_border.t, .border_b = target_border.b};
            list_data.target_cross = {
                .size = target_size.x, .margin_a = target_margin.l, .margin_b = target_margin.r, .border_a = target_border.l, .border_b = target_border.r};
        }
        return list_data;
    }
};

bool listCalculateMarginBorder(const bool main_is_x, const f32 max_size_main_f, const f32 max_size_cross_f, const f32 root_size_main_f,
                               const f32 root_size_cross_f, ChildUpdatingData::ListSizeData::CalcState& calc_state, auto& ret_px, auto& ret_flex,
                               const auto& target_margin, const bool is_main) {
    switch (target_margin.type) {
        case TargetBound::Type::PX: {
            ret_px = as<int>(target_margin.val);
            calc_state = ChildUpdatingData::ListSizeData::CalcState::DONE;
        } break;
        case TargetBound::Type::DEFAULT:
        case TargetBound::Type::RELATIVE: {
            ret_px = is_main ? as<int>(max_size_main_f * target_margin.val) : as<int>(max_size_cross_f * target_margin.val);
            calc_state = ChildUpdatingData::ListSizeData::CalcState::DONE;
        } break;
        case TargetBound::Type::ROOT: {
            ret_px = is_main ? as<int>(root_size_main_f * target_margin.val) : as<int>(root_size_cross_f * target_margin.val);
            calc_state = ChildUpdatingData::ListSizeData::CalcState::DONE;
        } break;
        case TargetBound::Type::RELATIVE_W: {
            f32 max_size_width_f = main_is_x ? max_size_main_f : max_size_cross_f;
            ret_px = as<int>(max_size_width_f * target_margin.val);
            calc_state = ChildUpdatingData::ListSizeData::CalcState::DONE;
        } break;
        case TargetBound::Type::RELATIVE_H: {
            f32 max_size_height_f = main_is_x ? max_size_cross_f : max_size_main_f;
            ret_px = as<int>(max_size_height_f * target_margin.val);
            calc_state = ChildUpdatingData::ListSizeData::CalcState::DONE;
        } break;
        case TargetBound::Type::FLEX_SHARE: {
            ret_flex = target_margin.val;
            calc_state = ChildUpdatingData::ListSizeData::CalcState::FLEX;
        } break;
        default: {
            MLE_W("Invalid margin target bound type for margin calculation: {}, treating as 0px", target_margin.type);
            ret_px = 0;
            calc_state = ChildUpdatingData::ListSizeData::CalcState::DONE;
        } break;
    };
    return calc_state == ChildUpdatingData::ListSizeData::CalcState::DONE;
};

bool listCalculateSize(const bool main_is_x, const f32 max_size_main_f, const f32 max_size_cross_f, const f32 root_size_main_f, const f32 root_size_cross_f,
                       ChildUpdatingData::ListSizeData::CalcState& calc_state, int& ret_px, f32& ret_flex, const TargetBound& target_size, const bool is_main,
                       const bool has_size_provider, const f32 aspect_ratio) {
    switch (target_size.type) {
        case TargetBound::Type::PX: {
            ret_px = as<int>(target_size.val);
            calc_state = ChildUpdatingData::ListSizeData::CalcState::DONE;
        } break;
        case TargetBound::Type::RELATIVE: {
            ret_px = is_main ? as<int>(max_size_main_f * target_size.val) : as<int>(max_size_cross_f * target_size.val);
            calc_state = ChildUpdatingData::ListSizeData::CalcState::DONE;
        } break;
        case TargetBound::Type::ROOT: {
            ret_px = is_main ? as<int>(root_size_main_f * target_size.val) : as<int>(root_size_cross_f * target_size.val);
            calc_state = ChildUpdatingData::ListSizeData::CalcState::DONE;
        } break;
        case TargetBound::Type::RELATIVE_W: {
            f32 max_size_width_f = main_is_x ? max_size_main_f : max_size_cross_f;
            ret_px = as<int>(max_size_width_f * target_size.val);
            calc_state = ChildUpdatingData::ListSizeData::CalcState::DONE;
        } break;
        case TargetBound::Type::RELATIVE_H: {
            f32 max_size_height_f = main_is_x ? max_size_cross_f : max_size_main_f;
            ret_px = as<int>(max_size_height_f * target_size.val);
            calc_state = ChildUpdatingData::ListSizeData::CalcState::DONE;
        } break;
        case TargetBound::Type::FLEX_SHARE: {
            ret_flex = target_size.val;
            calc_state = ChildUpdatingData::ListSizeData::CalcState::FLEX;
        } break;
        case TargetBound::Type::FIT: {
            calc_state = ChildUpdatingData::ListSizeData::CalcState::FIT;
        } break;
        case TargetBound::Type::DEFAULT: {
            if (aspect_ratio > 0) {
                calc_state = ChildUpdatingData::ListSizeData::CalcState::AR;
            } else if (has_size_provider) {
                calc_state = ChildUpdatingData::ListSizeData::CalcState::FIT;
            } else {
                ret_flex = target_size.val == 0.0F ? 1.0F : target_size.val;
                calc_state = ChildUpdatingData::ListSizeData::CalcState::FLEX;
            }
        } break;
        default: {
            MLE_W("Invalid size target bound type for size calculation: {}, treating as flex 1", target_size.type);
            ret_flex = 1.0F;
            calc_state = ChildUpdatingData::ListSizeData::CalcState::FLEX;
        } break;
    };
    return calc_state == ChildUpdatingData::ListSizeData::CalcState::DONE;
};

void listFinishFlexCross(auto& cld, const f32 max_size_cross) {
    f32 flex_acc_cross = cld.flex.size.cross + cld.flex.margin.cross_a + cld.flex.margin.cross_b + cld.flex.border.cross_a + cld.flex.border.cross_b;
    if (flex_acc_cross > 0.0F) {
        int px_cross =
            as<int>(max_size_cross) - cld.size_cross_px - cld.margin_px.cross_a - cld.margin_px.cross_b - cld.border_px.cross_a - cld.border_px.cross_b;
        px_cross = std::max(0, px_cross);
        f32 flex_share_size = as<f32>(px_cross) / std::max(1.F, flex_acc_cross);
        if (cld.state.cross == ChildUpdatingData::ListSizeData::CalcState::FLEX) {
            cld.size_cross_px = as<int>(cld.flex.size.cross * flex_share_size);
            cld.state.cross = ChildUpdatingData::ListSizeData::CalcState::DONE;
        }
        if (cld.state.margin_cross_a == ChildUpdatingData::ListSizeData::CalcState::FLEX) {
            cld.margin_px.cross_a = as<int>(cld.flex.margin.cross_a * flex_share_size);
            cld.state.margin_cross_a = ChildUpdatingData::ListSizeData::CalcState::DONE;
        }
        if (cld.state.margin_cross_b == ChildUpdatingData::ListSizeData::CalcState::FLEX) {
            cld.margin_px.cross_b = as<int>(cld.flex.margin.cross_b * flex_share_size);
            cld.state.margin_cross_b = ChildUpdatingData::ListSizeData::CalcState::DONE;
        }
        if (cld.state.border_cross_a == ChildUpdatingData::ListSizeData::CalcState::FLEX) {
            cld.border_px.cross_a = as<int>(cld.flex.border.cross_a * flex_share_size);
            cld.state.border_cross_a = ChildUpdatingData::ListSizeData::CalcState::DONE;
        }
        if (cld.state.border_cross_b == ChildUpdatingData::ListSizeData::CalcState::FLEX) {
            cld.border_px.cross_b = as<int>(cld.flex.border.cross_b * flex_share_size);
            cld.state.border_cross_b = ChildUpdatingData::ListSizeData::CalcState::DONE;
        }
    }
}

int minGapMainPx(const TargetBound& min_gap_main, const f32 root_size_main_px_f, const f32 max_size_main_px_f, const vec2i& padded_max_size_px) {
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

int maxCrossSizePx(const TargetBound& max_size_cross, const f32 root_size_cross_px_f, const vec2i& padded_max_size_px, const int container_size_cross_px) {
    switch (max_size_cross.type) {
        case TargetBound::Type::PX: {
            return as<int>(max_size_cross.val);
        } break;
        case TargetBound::Type::DEFAULT: {
            return container_size_cross_px;
        } break;
        case TargetBound::Type::RELATIVE: {
            return as<int>(as<f32>(container_size_cross_px) * max_size_cross.val);
        } break;
        case TargetBound::Type::ROOT: {
            return as<int>(root_size_cross_px_f * max_size_cross.val);
        } break;
        case TargetBound::Type::RELATIVE_W: {
            return as<int>(as<f32>(padded_max_size_px.x) * max_size_cross.val);
        };
        case TargetBound::Type::RELATIVE_H: {
            return as<int>(as<f32>(padded_max_size_px.y) * max_size_cross.val);
        } break;
        default: {
            // NOLINTNEXTLINE(bugprone-lambda-function-name) not a problem
            MLE_W("Invalid cross max size type: {}.", max_size_cross.type);
            return container_size_cross_px;
        } break;
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity) IDK if I can/should split this function
void calculateListChildrenSize(const Entt& e, const Container& self, const vec2i padded_max_size_px, const std::vector<entt::entity>& list_children,
                               std::map<entt::entity, ChildUpdatingData>& children_updating_data) {
    const auto main_is_x = self.list_direction == Container::ListDirection::HORIZONTAL || self.list_direction == Container::ListDirection::HORIZONTAL_REVERSED;

    const auto root_size_px = e.ui().getRootSize();

    const int root_size_main_px = as<int>(main_is_x ? root_size_px.x : root_size_px.y);
    const int root_size_cross_px = as<int>(main_is_x ? root_size_px.y : root_size_px.x);

    const f32 root_size_main_px_f = as<f32>(root_size_main_px);
    const f32 root_size_cross_px_f = as<f32>(root_size_cross_px);

    const int max_size_main_px = main_is_x ? padded_max_size_px.x : padded_max_size_px.y;

    const auto max_size_cross_px =
        maxCrossSizePx(self.max_size_cross, root_size_cross_px_f, padded_max_size_px, main_is_x ? padded_max_size_px.y : padded_max_size_px.x);

    const f32 max_size_main_px_f = as<f32>(max_size_main_px);
    const f32 max_size_cross_px_f = as<f32>(max_size_cross_px);

    const int min_gap_main_px = minGapMainPx(self.min_gap_main, root_size_main_px_f, max_size_main_px_f, padded_max_size_px);

    const int max_main_size_fit_px = [&]() {
        switch (self.max_fit_size_main.type) {
            case TargetBound::Type::PX: {
                return as<int>(self.max_fit_size_main.val);
            } break;
            case TargetBound::Type::ROOT: {
                return as<int>(root_size_main_px_f * self.max_fit_size_main.val);
            } break;
            case TargetBound::Type::DEFAULT: {
                return max<int>();
            } break;
            case TargetBound::Type::RELATIVE: {
                return as<int>(max_size_main_px_f * self.max_fit_size_main.val);
            } break;
            case TargetBound::Type::RELATIVE_W: {
                return as<int>(as<f32>(padded_max_size_px.x) * self.max_fit_size_main.val);
            } break;
            case TargetBound::Type::RELATIVE_H: {
                return as<int>(as<f32>(padded_max_size_px.y) * self.max_fit_size_main.val);
            } break;
            default: {
                // NOLINTNEXTLINE(bugprone-lambda-function-name) not a problem
                MLE_W("Invalid main max fit size type: {}. Returning max<int>().", self.max_fit_size_main.type);
                return max<int>();
            } break;
        }
    }();

    const int max_cross_size_fit_px = [&]() {
        switch (self.max_fit_size_cross.type) {
            case TargetBound::Type::PX: {
                return as<int>(self.max_fit_size_cross.val);
            } break;
            case TargetBound::Type::ROOT: {
                return as<int>(root_size_cross_px_f * self.max_fit_size_cross.val);
            } break;
            case TargetBound::Type::DEFAULT: {
                return max<int>();
            } break;
            case TargetBound::Type::RELATIVE: {
                return as<int>(max_size_cross_px_f * self.max_fit_size_cross.val);
            } break;
            case TargetBound::Type::RELATIVE_W: {
                return as<int>(as<f32>(padded_max_size_px.x) * self.max_fit_size_cross.val);
            } break;
            case TargetBound::Type::RELATIVE_H: {
                return as<int>(as<f32>(padded_max_size_px.y) * self.max_fit_size_cross.val);
            } break;
            default: {
                // NOLINTNEXTLINE(bugprone-lambda-function-name) not a problem
                MLE_W("Invalid cross max fit size type: {}. Returning max<int>().", self.max_fit_size_cross.type);
                return max<int>();
            } break;
        }
    }();

    f32 flex_acc_main = 0;
    int px_acc_main = 0;

    std::map<entt::entity, ChildUpdatingData::ListSizeData> list_children_data;

    for (auto c : list_children) {
        Entt centt{e.ui(), c};

        auto& cld = list_children_data[c];
        auto& cud = children_updating_data.at(c);
        cld = cud.gatterListData(main_is_x);

        bool done = listCalculateMarginBorder(main_is_x, max_size_main_px_f, max_size_cross_px_f, root_size_main_px_f, root_size_cross_px_f,
                                              cld.state.margin_main_a, cld.margin_px.main_a, cld.flex.margin.main_a, cld.target_main.margin_a, true);
        done &= listCalculateMarginBorder(main_is_x, max_size_main_px_f, max_size_cross_px_f, root_size_main_px_f, root_size_cross_px_f,
                                          cld.state.margin_main_b, cld.margin_px.main_b, cld.flex.margin.main_b, cld.target_main.margin_b, true);

        if ((cld.target_cross.margin_a.type != TargetBound::Type::DEFAULT && cld.target_cross.margin_a.val != 0.0F) ||
            (cld.target_cross.margin_b.type != TargetBound::Type::DEFAULT && cld.target_cross.margin_b.val != 0.0F)) {
            MLE_W("Non-default cross margin_b on list child is not supported and will be ignored.");
        }

        done &= listCalculateMarginBorder(main_is_x, max_size_main_px_f, max_size_cross_px_f, root_size_main_px_f, root_size_cross_px_f,
                                          cld.state.margin_cross_a, cld.margin_px.cross_a, cld.flex.margin.cross_a, cld.target_cross.margin_a, false);
        done &= listCalculateMarginBorder(main_is_x, max_size_main_px_f, max_size_cross_px_f, root_size_main_px_f, root_size_cross_px_f,
                                          cld.state.margin_cross_b, cld.margin_px.cross_b, cld.flex.margin.cross_b, cld.target_cross.margin_b, false);

        done &= listCalculateMarginBorder(main_is_x, max_size_main_px_f, max_size_cross_px_f, root_size_main_px_f, root_size_cross_px_f,
                                          cld.state.border_main_a, cld.border_px.main_a, cld.flex.border.main_a, cld.target_main.border_a, true);
        done &= listCalculateMarginBorder(main_is_x, max_size_main_px_f, max_size_cross_px_f, root_size_main_px_f, root_size_cross_px_f,
                                          cld.state.border_main_b, cld.border_px.main_b, cld.flex.border.main_b, cld.target_main.border_b, true);
        done &= listCalculateMarginBorder(main_is_x, max_size_main_px_f, max_size_cross_px_f, root_size_main_px_f, root_size_cross_px_f,
                                          cld.state.border_cross_a, cld.border_px.cross_a, cld.flex.border.cross_a, cld.target_cross.border_a, false);
        done &= listCalculateMarginBorder(main_is_x, max_size_main_px_f, max_size_cross_px_f, root_size_main_px_f, root_size_cross_px_f,
                                          cld.state.border_cross_b, cld.border_px.cross_b, cld.flex.border.cross_b, cld.target_cross.border_b, false);

        done &= listCalculateSize(main_is_x, max_size_main_px_f, max_size_cross_px_f, root_size_main_px_f, root_size_cross_px_f, cld.state.main,
                                  cld.size_main_px, cld.flex.size.main, cld.target_main.size, true, cud.size_provider != nullptr, cud.target_aspect_ratio);

        if (self.list_align_cross != Container::AlignCross::STRETCH) {
            done &=
                listCalculateSize(main_is_x, max_size_main_px_f, max_size_cross_px_f, root_size_main_px_f, root_size_cross_px_f, cld.state.cross,
                                  cld.size_cross_px, cld.flex.size.cross, cld.target_cross.size, false, cud.size_provider != nullptr, cud.target_aspect_ratio);
        } else {
            cld.size_cross_px = 0.0F;
            cld.flex.size.cross = 1.0F;
            cld.state.cross = ChildUpdatingData::ListSizeData::CalcState::FLEX;
        }

        switch (self.list_align_cross) {
            case Container::AlignCross::CENTER: {
                cld.flex.margin.cross_a = .01;
                cld.flex.margin.cross_b = .01;
            } break;
            case Container::AlignCross::END: {
                cld.flex.margin.cross_a = .02;
                cld.flex.margin.cross_b = .00;
            } break;
            default: {
                MLE_NOOP;
            } break;
        }

        if (done) {
            cld.done = true;
            flex_acc_main += cld.flex.size.main + cld.flex.margin.main_a + cld.flex.margin.main_b + cld.flex.border.main_a + cld.flex.border.main_b;
            px_acc_main += cld.size_main_px + cld.margin_px.main_a + cld.margin_px.main_b + cld.border_px.main_a + cld.border_px.main_b;
            continue;
        }

        if (cld.state.main == ChildUpdatingData::ListSizeData::CalcState::FLEX) {
            flex_acc_main += cld.flex.size.main + cld.flex.margin.main_a + cld.flex.margin.main_b + cld.flex.border.main_a + cld.flex.border.main_b;
            px_acc_main += cld.size_main_px + cld.margin_px.main_a + cld.margin_px.main_b + cld.border_px.main_a + cld.border_px.main_b;
            continue;
        }

        if (cld.state.main == ChildUpdatingData::ListSizeData::CalcState::FIT && cld.state.cross == ChildUpdatingData::ListSizeData::CalcState::FIT) {
            int max_size_main = max_main_size_fit_px - cld.margin_px.main_a - cld.margin_px.main_b - cld.border_px.main_a - cld.border_px.main_b;
            int max_size_cross = max_cross_size_fit_px - cld.margin_px.cross_a - cld.margin_px.cross_b - cld.border_px.cross_a - cld.border_px.cross_b;

            if (max_size_cross <= 0 || max_size_main <= 0) {
                MLE_E("Child cannot FIT in the given max size");
                cld.valid = false;
                continue;
            }

            vec2u p_max_size = main_is_x ? vec2u{max_size_main, max_size_cross} : vec2u{max_size_cross, max_size_main};
            vec2u size_from_provider = cud.size_provider->calc(e, p_max_size);
            u32 main_size_from_provider = main_is_x ? size_from_provider.x : size_from_provider.y;
            u32 cross_size_from_provider = main_is_x ? size_from_provider.y : size_from_provider.x;

            cld.size_main_px = as<int>(main_size_from_provider);
            cld.state.main = ChildUpdatingData::ListSizeData::CalcState::DONE;
            cld.size_cross_px = as<int>(cross_size_from_provider);
            cld.state.cross = ChildUpdatingData::ListSizeData::CalcState::DONE;
        }

        if (cld.state.cross == ChildUpdatingData::ListSizeData::CalcState::FIT) {
            if (cld.state.main == ChildUpdatingData::ListSizeData::CalcState::DONE) {
                int max_size_cross = max_cross_size_fit_px - cld.margin_px.cross_a - cld.margin_px.cross_b - cld.border_px.cross_a - cld.border_px.cross_b;
                if (max_size_cross <= 0) {
                    MLE_E("Child cannot FIT in the given max cross size");
                    cld.valid = false;
                    continue;
                }

                vec2u p_max_size = main_is_x ? vec2u{cld.size_main_px, max_size_cross} : vec2u{max_size_cross, cld.size_main_px};
                vec2u size_from_provider = cud.size_provider->calc(e, p_max_size);
                u32 cross_size_from_provider = main_is_x ? size_from_provider.y : size_from_provider.x;

                cld.size_cross_px = as<int>(cross_size_from_provider);
                cld.state.cross = ChildUpdatingData::ListSizeData::CalcState::DONE;
            } else if (cld.state.main == ChildUpdatingData::ListSizeData::CalcState::AR) {
                MLE_E("Cannot mix FIT and AR");
                cld.valid = false;
                continue;
            }
        }

        if (cld.state.cross == ChildUpdatingData::ListSizeData::CalcState::AR) {
            if (cld.state.main == ChildUpdatingData::ListSizeData::CalcState::DONE) {
                if (main_is_x) {
                    cld.size_cross_px = as<int>(as<f32>(cld.size_main_px) / cud.target_aspect_ratio);
                } else {
                    cld.size_cross_px = as<int>(as<f32>(cld.size_main_px) * cud.target_aspect_ratio);
                }
                cld.state.cross = ChildUpdatingData::ListSizeData::CalcState::DONE;
            } else if (cld.state.main == ChildUpdatingData::ListSizeData::CalcState::FIT) {
                MLE_E("Cannot mix FIT and AR");
                cld.valid = false;
                continue;
            } else if (cld.state.main == ChildUpdatingData::ListSizeData::CalcState::AR) {
                MLE_E("Cannot have AR on both axis.");
                cld.valid = false;
                continue;
            }
        }

        listFinishFlexCross(cld, max_size_cross_px_f);

        if (cld.state.main == ChildUpdatingData::ListSizeData::CalcState::AR) {
            if (cld.state.cross == ChildUpdatingData::ListSizeData::CalcState::DONE) {
                if (main_is_x) {
                    cld.size_main_px = as<int>(as<f32>(cld.size_cross_px) * cud.target_aspect_ratio);
                } else {
                    cld.size_main_px = as<int>(as<f32>(cld.size_cross_px) / cud.target_aspect_ratio);
                }
                cld.state.main = ChildUpdatingData::ListSizeData::CalcState::DONE;
            } else {
                MLE_UNREACHABLE_LOG("This should be unreachable due to previous checks. Main state: AR, Cross state:{}", (int)cld.state.cross);
                cld.valid = false;
                continue;
            }
        }

        flex_acc_main += cld.flex.size.main + cld.flex.margin.main_a + cld.flex.margin.main_b + cld.flex.border.main_a + cld.flex.border.main_b;
        px_acc_main += cld.size_main_px + cld.margin_px.main_a + cld.margin_px.main_b + cld.border_px.main_a + cld.border_px.main_b;
    }

    px_acc_main += list_children.size() > 0 ? min_gap_main_px * (as<int>(list_children.size()) - 1) : 0;

    int remaining_px_main = as<int>(max_size_main_px) - px_acc_main;
    remaining_px_main = std::max(0, remaining_px_main);
    f32 flex_share_main = (flex_acc_main > 0.0F) ? as<f32>(remaining_px_main) / std::max(1.F, flex_acc_main) : 0.0F;

    for (auto c : list_children) {
        auto& cld = list_children_data[c];
        if (!cld.valid) {
            continue;
        }
        auto& cud = children_updating_data.at(c);

        if (!cld.done) {
            if (cld.state.main == ChildUpdatingData::ListSizeData::CalcState::FLEX) {
                cld.size_main_px = as<int>(cld.flex.size.main * flex_share_main);
                cld.state.main = ChildUpdatingData::ListSizeData::CalcState::DONE;
            }
            if (cld.state.margin_main_a == ChildUpdatingData::ListSizeData::CalcState::FLEX) {
                cld.margin_px.main_a = as<int>(cld.flex.margin.main_a * flex_share_main);
                cld.state.margin_main_a = ChildUpdatingData::ListSizeData::CalcState::DONE;
            }
            if (cld.state.margin_main_b == ChildUpdatingData::ListSizeData::CalcState::FLEX) {
                cld.margin_px.main_b = as<int>(cld.flex.margin.main_b * flex_share_main);
                cld.state.margin_main_b = ChildUpdatingData::ListSizeData::CalcState::DONE;
            }
            if (cld.state.border_main_a == ChildUpdatingData::ListSizeData::CalcState::FLEX) {
                cld.border_px.main_a = as<int>(cld.flex.border.main_a * flex_share_main);
                cld.state.border_main_a = ChildUpdatingData::ListSizeData::CalcState::DONE;
            }
            if (cld.state.border_main_b == ChildUpdatingData::ListSizeData::CalcState::FLEX) {
                cld.border_px.main_b = as<int>(cld.flex.border.main_b * flex_share_main);
                cld.state.border_main_b = ChildUpdatingData::ListSizeData::CalcState::DONE;
            }
            // if cross is ar
            if (cld.state.cross == ChildUpdatingData::ListSizeData::CalcState::AR) {
                if (main_is_x) {
                    cld.size_cross_px = as<int>(as<f32>(cld.size_main_px) / cud.target_aspect_ratio);
                } else {
                    cld.size_cross_px = as<int>(as<f32>(cld.size_main_px) * cud.target_aspect_ratio);
                }
                cld.state.cross = ChildUpdatingData::ListSizeData::CalcState::DONE;
            }
            listFinishFlexCross(cld, max_size_cross_px_f);

            cld.done = cld.state.main == ChildUpdatingData::ListSizeData::CalcState::DONE &&
                       cld.state.cross == ChildUpdatingData::ListSizeData::CalcState::DONE &&
                       cld.state.margin_main_a == ChildUpdatingData::ListSizeData::CalcState::DONE &&
                       cld.state.margin_main_b == ChildUpdatingData::ListSizeData::CalcState::DONE &&
                       cld.state.margin_cross_a == ChildUpdatingData::ListSizeData::CalcState::DONE &&
                       cld.state.margin_cross_b == ChildUpdatingData::ListSizeData::CalcState::DONE &&
                       cld.state.border_main_a == ChildUpdatingData::ListSizeData::CalcState::DONE &&
                       cld.state.border_main_b == ChildUpdatingData::ListSizeData::CalcState::DONE &&
                       cld.state.border_cross_a == ChildUpdatingData::ListSizeData::CalcState::DONE &&
                       cld.state.border_cross_b == ChildUpdatingData::ListSizeData::CalcState::DONE;

            if (!cld.done) {
                MLE_E("Could not resolve all size/margin/border for list child. Please fixe me!");
                cld.valid = false;
                continue;
            }
        }

        MLE_ASSERT_LOG(cld.size_main_px > 0 && cld.size_cross_px > 0, "Negative or zero size calculated for list child. main: {}, cross: {}", cld.size_main_px,
                       cld.size_cross_px);

        if (main_is_x) {
            cud.new_size.x = cld.size_main_px;
            cud.new_size.y = cld.size_cross_px;
            cud.new_margin.t = cld.margin_px.cross_a;
            cud.new_margin.b = cld.margin_px.cross_b;
            cud.new_margin.l = cld.margin_px.main_a;
            cud.new_margin.r = cld.margin_px.main_b;
            cud.new_border.t = cld.border_px.cross_a;
            cud.new_border.b = cld.border_px.cross_b;
            cud.new_border.l = cld.border_px.main_a;
            cud.new_border.r = cld.border_px.main_b;
        } else {
            cud.new_size.y = cld.size_main_px;
            cud.new_size.x = cld.size_cross_px;
            cud.new_margin.l = cld.margin_px.cross_a;
            cud.new_margin.r = cld.margin_px.cross_b;
            cud.new_margin.t = cld.margin_px.main_a;
            cud.new_margin.b = cld.margin_px.main_b;
            cud.new_border.l = cld.border_px.cross_a;
            cud.new_border.r = cld.border_px.cross_b;
            cud.new_border.t = cld.border_px.main_a;
            cud.new_border.b = cld.border_px.main_b;
        }
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity) This CANT be splittd, its just a bunch of consts
void calculateListChildrenPosition(const Entt& e, const Container& self, const vec2i padded_max_size_px, const std::vector<entt::entity>& list_children,
                                   std::map<entt::entity, ChildUpdatingData>& children_updating_data) {
    const bool main_is_x = self.list_direction == Container::ListDirection::HORIZONTAL || self.list_direction == Container::ListDirection::HORIZONTAL_REVERSED;

    // FIXME: FIX THIS STUPID NAMING
    const bool main_reversed =
        self.list_direction == Container::ListDirection::HORIZONTAL_REVERSED || self.list_direction == Container::ListDirection::VERTICAL_REVERSED;
    const bool cross_reversed = self.wrap_mode == Container::WrapMode::WRAP_REVERSED;

    const auto root_size_px = e.ui().getRootSize();

    const int root_size_main_px = as<int>(main_is_x ? root_size_px.x : root_size_px.y);
    const int root_size_cross_px = as<int>(main_is_x ? root_size_px.y : root_size_px.x);

    const f32 root_size_main_px_f = as<f32>(root_size_main_px);
    const f32 root_size_cross_px_f = as<f32>(root_size_cross_px);

    const int max_size_main_px = main_is_x ? padded_max_size_px.x : padded_max_size_px.y;
    const int container_size_cross_px = main_is_x ? padded_max_size_px.y : padded_max_size_px.x;

    const auto max_size_cross_px = maxCrossSizePx(self.max_size_cross, root_size_cross_px_f, padded_max_size_px, container_size_cross_px);

    const f32 max_size_main_px_f = as<f32>(max_size_main_px);

    const int min_gap_main_px = minGapMainPx(self.min_gap_main, root_size_main_px_f, max_size_main_px_f, padded_max_size_px);
    const int line_gap_cross_px = 0;  // TODO: this

    const auto container_origin_px_main = main_reversed ? max_size_main_px : 0;
    const auto container_origin_px_cross = main_reversed ? container_size_cross_px : 0;

    const auto base_pos_main_px = container_origin_px_main;
    auto current_pos_cross_px = container_origin_px_cross;

    std::vector<int> children_main_sizes_px;
    for (auto c : list_children) {
        auto& cud = children_updating_data.at(c);
        auto total_margin_border_main = main_is_x ? (cud.new_margin.l + cud.new_margin.r + cud.new_border.l + cud.new_border.r)
                                                  : (cud.new_margin.t + cud.new_margin.b + cud.new_border.t + cud.new_border.b);
        auto total_size_main = main_is_x ? cud.new_size.x : cud.new_size.y;
        total_size_main += total_margin_border_main;
        children_main_sizes_px.push_back(total_size_main);
    }

    switch (self.wrap_mode) {
        case Container::WrapMode::NO: {
            auto line = JustifyInt::justifyUntilOverflow(children_main_sizes_px, min_gap_main_px, self.list_align_main, self.list_align_main, max_size_main_px);
            if (self.scrollable && line.size() != list_children.size()) {
                line = JustifyInt::justifyLineStart(children_main_sizes_px, min_gap_main_px);
            }
            for (usize i = 0; i < line.size(); i++) {
                auto centt = Entt{e.ui(), list_children[i]};
                auto& cud = children_updating_data.at(centt.e());
                auto pos_main_beg = line[i];
                auto c_size_main = children_main_sizes_px[i];
                auto pos_main_end = pos_main_beg + c_size_main;
                auto new_pos_main = base_pos_main_px + (main_reversed ? -pos_main_end : pos_main_beg);

                cud.new_position = main_is_x ? vec2i{new_pos_main, current_pos_cross_px} : vec2i{current_pos_cross_px, new_pos_main};
                cud.new_position.x -= cud.new_margin.l + cud.new_border.l;
                cud.new_position.y -= cud.new_margin.t + cud.new_border.t;
            }

        } break;
        case Container::WrapMode::WRAP_REVERSED:
        case Container::WrapMode::WRAP: {
            auto lines = JustifyInt::wrap(children_main_sizes_px, min_gap_main_px, self.list_align_main, JustifyInt::LineMode::START, max_size_main_px);
            usize line_index = 0;
            usize child_index_in_line = 0;
            for (usize i = 0; i < list_children.size(); i++) {
                auto centt = Entt{e.ui(), list_children[i]};
                auto& cud = children_updating_data.at(centt.e());
                auto pos_main_beg = lines[line_index][child_index_in_line];
                auto c_size_main = children_main_sizes_px[i];
                auto pos_main_end = pos_main_beg + c_size_main;
                auto new_pos_main = base_pos_main_px + (main_reversed ? -pos_main_end : pos_main_beg);

                cud.new_position = main_is_x ? vec2i{new_pos_main, current_pos_cross_px} : vec2i{current_pos_cross_px, new_pos_main};
                cud.new_position.x -= cud.new_margin.l + cud.new_border.l;
                cud.new_position.y -= cud.new_margin.t + cud.new_border.t;

                child_index_in_line++;
                if (child_index_in_line >= lines[line_index].size()) {
                    line_index++;
                    child_index_in_line = 0;
                    current_pos_cross_px += cross_reversed ? -(max_size_cross_px + line_gap_cross_px) : (max_size_cross_px + line_gap_cross_px);
                }
            }
        } break;
        default: {
            MLE_UNREACHABLE_LOG("Invalid wrap mode: {}", (int)self.wrap_mode);
        } break;
    }
}

vec2u accumulateChildrenBounds(const std::map<entt::entity, ChildUpdatingData>& children_data, const PaddingPx& padding) {
    if (children_data.empty()) {
        return vec2u{0, 0};
    }

    vec2i min_pos{std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
    vec2i max_pos{std::numeric_limits<int>::min(), std::numeric_limits<int>::min()};

    for (const auto& [_, cud] : children_data) {
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

void finishChildBounds(const Entt& e, auto& cud) {
    MLE_C("Finishing {} bounds", e.name());

    comp::Bounds new_bounds;
    new_bounds.parent_px.setPos(cud.new_position);
    new_bounds.parent_px.setSize(cud.new_size);

    MLE_VC(new_bounds);

    e.emplaceOrReplace<comp::Bounds>(new_bounds);

    if (e.has<comp::TargetBorder>()) {
        comp::Border new_border;
        new_border.t = cud.new_border.t;
        new_border.b = cud.new_border.b;
        new_border.l = cud.new_border.l;
        new_border.r = cud.new_border.r;
        new_border.color = cud.target_border.color;

        auto largest_size = std::max({cud.new_size.x, cud.new_size.y});
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
                    return as<int>(as<f32>(cud.new_size.x) * round_tb.val);
                } break;
                case TargetBound::Type::RELATIVE_H: {
                    return as<int>(as<f32>(cud.new_size.y) * round_tb.val);
                } break;
                default: {
                    // NOLINTNEXTLINE(bugprone-lambda-function-name) not a problem
                    MLE_W("Invalid border round type: {}. Treating as 0.", round_tb.type);
                    return 0;
                } break;
            }
        };
        new_border.round_lt = roundc(cud.target_border.round_lt);
        new_border.round_rt = roundc(cud.target_border.round_rt);
        new_border.round_lb = roundc(cud.target_border.round_lb);
        new_border.round_rb = roundc(cud.target_border.round_rb);

        MLE_VC(new_border);

        e.emplaceOrReplace<comp::Border>(new_border);
    }
}

void finishChildrenBounds(const Entt& e, auto& children_data, std::span<const entt::entity> to_update) {
    for (auto c : to_update) {
        auto centt = Entt(e.ui(), c);
        auto& cud = children_data.at(c);

        finishChildBounds(centt, cud);
    }
}

void calculateListChildrenBounds(const Entt& e, const Container& self, const vec2i padded_max_size_px, const std::vector<entt::entity>& list_children,
                                 std::map<entt::entity, ChildUpdatingData>& children_updating_data) {
    calculateListChildrenSize(e, self, padded_max_size_px, list_children, children_updating_data);
    calculateListChildrenPosition(e, self, padded_max_size_px, list_children, children_updating_data);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity) Yeah
void calculateFreeChildrenBounds(const Entt& e, const Container& self, const vec2i padded_max_size_px, const std::vector<entt::entity>& free_children,
                                 std::map<entt::entity, ChildUpdatingData>& children_updating_data) {
    auto sorted_by_dependencies = sortChildrenByDependency(e.ui(), free_children);

    const auto root_size_px = e.ui().getRootSize();
    const auto root_size_px_f = vec2f{as<f32>(root_size_px.x), as<f32>(root_size_px.y)};

    for (int i = 0; i < as<int>(sorted_by_dependencies.size()); i++) {
        auto c = sorted_by_dependencies[i];
        Entt centt{e.ui(), c};
        auto& cud = children_updating_data.at(c);

        switch (cud.target_position.x.type) {
            case TargetBound::Type::PX: {
                cud.new_position.x = as<int>(cud.target_position.x.val);
            } break;
            case TargetBound::Type::ROOT: {
                cud.new_position.x = as<int>(root_size_px_f.x * cud.target_position.x.val);
            } break;
            case TargetBound::Type::DEFAULT:
            case TargetBound::Type::RELATIVE_W:
            case TargetBound::Type::RELATIVE: {
                cud.new_position.x = as<int>(as<f32>(padded_max_size_px.x) * cud.target_position.x.val);
            } break;
            case TargetBound::Type::RELATIVE_H: {
                cud.new_position.x = as<int>(as<f32>(padded_max_size_px.y) * cud.target_position.x.val);
            } break;
            default: {
                MLE_W("Invalid target position x type: {}.", cud.target_position.x.type);
            } break;
        }

        if (cud.target_position.xdep.e != entt::null) {
            if (!self.o.getIdx(cud.target_position.xdep.e)) {
                MLE_W("Child {} has invalid y dependency on entity {}. Ignoring it.", centt.name(), as<u32>(cud.target_position.ydep.e));
                continue;
            }
            const auto dep_entt = Entt{e.ui(), cud.target_position.ydep.e};
            const auto tb = cud.target_position.xdep.dep_tb;
            auto& dep_bounds = dep_entt.get<comp::Bounds>();
            cud.new_position.x += dep_bounds.parent_px.left();
            switch (tb.type) {
                case TargetBound::Type::PX: {
                    cud.new_position.x += as<int>(tb.val) + dep_bounds.parent_px.width();
                } break;
                case TargetBound::Type::DEFAULT:
                case TargetBound::Type::RELATIVE:
                case TargetBound::Type::RELATIVE_W: {
                    cud.new_position.x += as<int>(as<f32>(dep_bounds.parent_px.width()) * tb.val);
                } break;
                default: {
                    MLE_W("Invalid x dependency target bound type: {}. Ignoring extra.", tb.type);
                } break;
            }
        }

        switch (cud.target_position.y.type) {
            case TargetBound::Type::PX: {
                cud.new_position.y = as<int>(cud.target_position.y.val);
            } break;
            case TargetBound::Type::ROOT: {
                cud.new_position.y = as<int>(root_size_px_f.y * cud.target_position.y.val);
            } break;
            case TargetBound::Type::DEFAULT:
            case TargetBound::Type::RELATIVE_W:
            case TargetBound::Type::RELATIVE: {
                cud.new_position.y = as<int>(as<f32>(padded_max_size_px.y) * cud.target_position.y.val);
            } break;
            case TargetBound::Type::RELATIVE_H: {
                cud.new_position.y = as<int>(as<f32>(padded_max_size_px.x) * cud.target_position.y.val);
            } break;
            default: {
                MLE_W("Invalid target position y type: {}.", cud.target_position.y.type);
            } break;
        }

        if (cud.target_position.ydep.e != entt::null) {
            if (!self.o.getIdx(cud.target_position.ydep.e)) {
                MLE_W("Child {} has invalid y dependency on entity {}. Ignoring it.", centt.name(), as<u32>(cud.target_position.ydep.e));
                continue;
            }
            const auto dep_entt = Entt{e.ui(), cud.target_position.ydep.e};
            const auto tb = cud.target_position.ydep.dep_tb;
            auto& dep_bounds = dep_entt.get<comp::Bounds>();
            cud.new_position.y += dep_bounds.parent_px.top();
            switch (tb.type) {
                case TargetBound::Type::PX: {
                    cud.new_position.y += as<int>(tb.val) + dep_bounds.parent_px.height();
                } break;
                case TargetBound::Type::DEFAULT:
                case TargetBound::Type::RELATIVE:
                case TargetBound::Type::RELATIVE_W: {
                    cud.new_position.y += as<int>(as<f32>(dep_bounds.parent_px.height()) * tb.val);
                } break;
                default: {
                    MLE_W("Invalid y dependency target bound type: {}. Ignoring extra.", tb.type);
                } break;
            }
        }

        bool x_is_ar = false;
        bool x_is_fit = false;
        bool y_is_fit = false;

        switch (cud.target_size.x.type) {
            case TargetBound::Type::PX: {
                cud.new_size.x = as<int>(cud.target_size.x.val);
            } break;
            case TargetBound::Type::ROOT: {
                cud.new_size.x = as<int>(root_size_px_f.x * cud.target_size.x.val);
            } break;
            case TargetBound::Type::RELATIVE_W:
            case TargetBound::Type::RELATIVE: {
                cud.new_size.x = as<int>(as<f32>(padded_max_size_px.x) * cud.target_size.x.val);
            }; break;
            case TargetBound::Type::RELATIVE_H: {
                cud.new_size.x = as<int>(as<f32>(padded_max_size_px.y) * cud.target_size.x.val);
            } break;
            case TargetBound::Type::FLEX_SHARE: {
                auto remaining_px = padded_max_size_px.x - cud.new_position.x;
                cud.new_size.x = as<int>(cud.target_size.x.val * as<f32>(remaining_px));
            } break;
            case TargetBound::Type::FIT: {
                if (cud.size_provider == nullptr) {
                    MLE_E("FIT target size x requires a size provider. Entity name:{}", e.name());
                    continue;
                }
                x_is_fit = true;
            } break;
            case TargetBound::Type::DEFAULT: {
                if (cud.target_size.x.val != 0) {
                    cud.new_size.x = as<int>(as<f32>(padded_max_size_px.x) * cud.target_size.x.val);
                } else if (cud.target_size.xdep.e == entt::null) {
                    if (cud.target_aspect_ratio > 0) {
                        x_is_ar = true;
                    } else if (cud.size_provider != nullptr) {
                        x_is_fit = true;
                    } else {
                        auto remaining_px = padded_max_size_px.x - cud.new_position.x;
                        cud.new_size.x = as<int>(as<f32>(remaining_px));
                    }
                }
            } break;
            default: {
                MLE_E("Weird enum value for target size x type: {}.", (int)cud.target_size.x.type);
                continue;
            } break;
        }

        if (cud.target_size.xdep.e != entt::null) {
            if (!self.o.getIdx(cud.target_size.xdep.e)) {
                MLE_W("Child {} has invalid x size dependency on entity {}. Ignoring it.", centt.name(), as<u32>(cud.target_size.xdep.e));
                continue;
            }
            const auto dep_entt = Entt{e.ui(), cud.target_size.xdep.e};
            const auto tb = cud.target_size.xdep.dep_tb;
            auto& dep_bounds = dep_entt.get<comp::Bounds>();
            switch (tb.type) {
                case TargetBound::Type::PX: {
                    cud.new_size.x += as<int>(tb.val) + dep_bounds.parent_px.width();
                } break;
                case TargetBound::Type::DEFAULT:
                case TargetBound::Type::RELATIVE:
                case TargetBound::Type::RELATIVE_W: {
                    cud.new_size.x += as<int>(as<f32>(dep_bounds.parent_px.width()) * tb.val);
                } break;
                default: {
                    MLE_W("Invalid x size dependency target bound type: {}. Ignoring extra.", tb.type);
                } break;
            }
        }

        switch (cud.target_size.y.type) {
            case TargetBound::Type::PX: {
                cud.new_size.y = as<int>(cud.target_size.y.val);
            } break;
            case TargetBound::Type::ROOT: {
                cud.new_size.y = as<int>(root_size_px_f.y * cud.target_size.y.val);
            }
            case TargetBound::Type::RELATIVE_H:
            case TargetBound::Type::RELATIVE: {
                cud.new_size.y = as<int>(as<f32>(padded_max_size_px.y) * cud.target_size.y.val);
            }; break;
            case TargetBound::Type::RELATIVE_W: {
                cud.new_size.y = as<int>(as<f32>(padded_max_size_px.x) * cud.target_size.y.val);
            } break;
            case TargetBound::Type::FLEX_SHARE: {
                auto remaining_px = padded_max_size_px.y - cud.new_position.y;
                cud.new_size.y = as<int>(cud.target_size.y.val * as<f32>(remaining_px));
            } break;
            case TargetBound::Type::FIT: {
                if (x_is_ar) {
                    MLE_E("Cannot mix FIT and AR.");
                    continue;
                }
                if (cud.size_provider == nullptr) {
                    MLE_E("FIT target size y requires a size provider. Entity name:{}", e.name());
                    continue;
                }
                y_is_fit = true;
            } break;
            case TargetBound::Type::DEFAULT: {
                if (cud.target_size.y.val != 0) {
                    cud.new_size.y = as<int>(as<f32>(padded_max_size_px.y) * cud.target_size.y.val);
                } else if (cud.target_size.ydep.e == entt::null) {
                    if (cud.size_provider != nullptr && !x_is_ar) {
                        y_is_fit = true;
                    } else {
                        auto remaining_px = padded_max_size_px.y - cud.new_position.y;
                        cud.new_size.y = as<int>(as<f32>(remaining_px));
                    }
                }
            } break;
            default: {
                MLE_E("Weird enum value for target size y type: {}.", (int)cud.target_size.y.type);
                continue;
            } break;
        }

        if (cud.target_size.ydep.e != entt::null) {
            if (!self.o.getIdx(cud.target_size.ydep.e)) {
                MLE_W("Child {} has invalid y size dependency on entity {}. Ignoring it.", centt.name(), as<u32>(cud.target_size.ydep.e));
                continue;
            }
            const auto dep_entt = Entt{e.ui(), cud.target_size.ydep.e};
            const auto tb = cud.target_size.ydep.dep_tb;
            auto& dep_bounds = dep_entt.get<comp::Bounds>();
            switch (tb.type) {
                case TargetBound::Type::PX: {
                    cud.new_size.y += as<int>(tb.val) + dep_bounds.parent_px.height();
                } break;
                case TargetBound::Type::DEFAULT:
                case TargetBound::Type::RELATIVE:
                case TargetBound::Type::RELATIVE_W: {
                    cud.new_size.y += as<int>(as<f32>(dep_bounds.parent_px.height()) * tb.val);
                } break;
                default: {
                    MLE_W("Invalid y size dependency target bound type: {}. Ignoring extra.", tb.type);
                } break;
            }
        }

        if (x_is_ar) {
            cud.new_size.x = as<int>(as<f32>(cud.new_size.y) * cud.target_aspect_ratio);
        }

        if (x_is_fit || y_is_fit) {
            vec2u fit_max_size_px = {cud.new_size.x, cud.new_size.y};

            if (fit_max_size_px.x <= 0) {
                fit_max_size_px.x = padded_max_size_px.x - cud.new_position.x;
            }
            if (fit_max_size_px.y <= 0) {
                fit_max_size_px.y = padded_max_size_px.y - cud.new_position.y;
            }

            vec2u size_from_provider = cud.size_provider->calc(e, fit_max_size_px);

            if (x_is_fit) {
                cud.new_size.x = as<int>(size_from_provider.x);
            }
            if (y_is_fit) {
                cud.new_size.y = as<int>(size_from_provider.y);
            }
        }

        finishChildBounds(centt, cud);
    }
}
}  // namespace

vec2u Container::calculateChildrenBounds(const Entt& e, vec2u max_size_px) const {
    auto children = o.get();

    const auto* padding_c = e.tryGet<comp::TargetPadding>();
    PaddingPx padding{};
    vec2i padded_max_size_px = vec2i{max_size_px};
    if (padding_c) {
        padding = padding_c->calc(e.ui(), max_size_px);
        padded_max_size_px.x -= padding.l + padding.r;
        padded_max_size_px.y -= padding.t + padding.b;
    }

    std::map<entt::entity, ChildUpdatingData> children_data;
    for (const auto& c : children) {
        Entt centt{e.ui(), c.e};
        children_data.emplace(c.e, centt);
    }

    switch (type) {
        case Type::HYBRID: {
            auto [free_children, list_children] = separateFreeListChildren(e.ui(), children);
            calculateListChildrenBounds(e, *this, padded_max_size_px, list_children, children_data);
            finishChildrenBounds(e, children_data, list_children);
            calculateFreeChildrenBounds(e, *this, padded_max_size_px, free_children, children_data);
        } break;
        case Type::LIST: {
            std::vector<entt::entity> list_children;
            for (const auto& c : children) {
                list_children.push_back(c.e);
            }
            calculateListChildrenBounds(e, *this, padded_max_size_px, list_children, children_data);
            finishChildrenBounds(e, children_data, list_children);
        } break;
        case Type::FREE: {
            std::vector<entt::entity> free_children;
            for (const auto& c : children) {
                free_children.push_back(c.e);
            }
            calculateFreeChildrenBounds(e, *this, padded_max_size_px, free_children, children_data);
        } break;
            break;
    }

    auto acc_children_bounds = accumulateChildrenBounds(children_data, padding);

    return acc_children_bounds;
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
}  // namespace mle::ui::comp
