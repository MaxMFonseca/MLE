#include "Container.h"

#include "mle/lua/Utils.h"
#include "mle/renderer/Types.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Bounds.h"
#include "mle/utils/ECS.h"

namespace mle::ui::comp {
Container::Container(const Entt& e, const sol::object& obj) {
    auto table = lua::as<sol::table>(obj);
    for (const auto& [key, value] : table) {
        if (key.is<std::string>()) {
            addChild(e, key.as<std::string>(), value);
        } else {
            addChild(e, "", value);
        }
    }
    setAsBoundsCalculator(e);
}

void Container::addChild(const Entt& e, std::string name, const sol::object& obj, usize pos) {
    auto child = e.ui().getLuaElementOps().createElement(obj, e.e());
    o.add(std::move(name), child, pos);
}

void Container::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    if (e.has<Container>()) {
        e.remove<Container>();  // remove cuz no copy move
    }
    e.emplace<Container>(e, obj);
}

void Container::applyAdd(const Entt& e, const sol::object& obj) {
    auto table = lua::as<sol::table>(obj);

    std::string name;
    auto name_r = lua::tryGetKeyOrIdx(table, "name", 1, name);
    MLE_ASSERT(name_r);

    sol::table child_table;
    auto child_table_r = lua::tryGetKeyOrIdx(table, "child", 2, child_table);
    MLE_ASSERT(child_table_r);

    auto pos = max<usize>();
    std::ignore = lua::tryGetKeyOrIdx(table, "pos", 3, pos);

    if (e.has<Container>()) {
        e.patch<Container>([&](Container& c) { c.addChild(e, std::move(name), child_table, pos); });
    } else {
        auto& new_c = e.emplace<Container>();
        new_c.addChild(e, std::move(name), child_table, pos);
    }
}

void Container::setAsBoundsCalculator(const Entt& e) {
    e.emplaceOrReplace<SizeProvider>([](const Entt& e, vec2u max_size) {
        auto& self = e.get<Container>();
        return self.calculateChildrenBounds(e, max_size);
    });
};

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

std::pair<std::vector<entt::entity>, std::vector<entt::entity>> separateFlexListChildren(UI& ui, std::span<const EntityStorage::Entry> span) {
    std::vector<entt::entity> flex_children;
    std::vector<entt::entity> list_children;

    for (const auto& e : span) {
        Entt ee{ui, e.e};
        if (ee.has<comp::TargetPosition>()) {
            flex_children.push_back(e.e);
        } else {
            list_children.push_back(e.e);
        }
    }

    return {flex_children, list_children};
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
        u32 t, b, l, r;
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
        if (const auto* target_size = e.tryGet<comp::TargetSize>(); target_size) {
            this->target_size = *target_size;
        }
        if (const auto* target_position = e.tryGet<comp::TargetPosition>(); target_position) {
            this->target_position = *target_position;
        }
        if (const auto* target_margin = e.tryGet<comp::TargetMargin>(); target_margin) {
            this->target_margin = *target_margin;
        }
        if (const auto* target_border = e.tryGet<comp::TargetBorder>(); target_border) {
            this->target_border = *target_border;
        }
        if (const auto* aspect_ratio = e.tryGet<comp::TargetAspectRatio>(); aspect_ratio) {
            target_aspect_ratio = aspect_ratio->o;
        }
        if (const auto* origin = e.tryGet<comp::TargetOrigin>(); origin) {
            target_origin = origin->o;
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
                ret_flex = 1.0F;
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
            as<int>(max_size_cross) - cld.size_main_px - cld.margin_px.cross_a - cld.margin_px.cross_b - cld.border_px.cross_a - cld.border_px.cross_b;
        px_cross = std::max(0, px_cross);
        f32 flex_share_size = as<f32>(px_cross) / flex_acc_cross;
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity) IDK if I can/should split this function
void calculateListChildrenSize(const Entt& e, const Container& self, const vec2i padded_max_size_px, const std::vector<entt::entity>& list_children,
                               std::map<entt::entity, ChildUpdatingData>& children_updating_data) {
    const auto main_is_x = self.list_direction == Container::ListDirection::HORIZONTAL || self.list_direction == Container::ListDirection::HORIZONTAL_REVERSED;

    const int max_size_main_px = main_is_x ? padded_max_size_px.x : padded_max_size_px.y;
    const int max_size_cross_px = main_is_x ? padded_max_size_px.y : padded_max_size_px.x;

    const f32 max_size_main_px_f = as<f32>(max_size_main_px);
    const f32 max_size_cross_px_f = as<f32>(max_size_cross_px);

    const auto root_size_px = e.ui().getRootSize();

    const int root_size_main_px = as<int>(main_is_x ? root_size_px.x : root_size_px.y);
    const int root_size_cross_px = as<int>(main_is_x ? root_size_px.y : root_size_px.x);

    const f32 root_size_main_px_f = as<f32>(root_size_main_px);
    const f32 root_size_cross_px_f = as<f32>(root_size_cross_px);

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

    const int min_gap_main_px = [&]() {
        switch (self.min_gap_main.type) {
            case TargetBound::Type::PX: {
                return as<int>(self.min_gap_main.val);
            }
            case TargetBound::Type::ROOT: {
                return as<int>(root_size_main_px_f * self.min_gap_main.val);
            } break;
            case TargetBound::Type::DEFAULT: {
                return as<int>(self.min_gap_main.val);
            } break;
            case TargetBound::Type::RELATIVE: {
                return as<int>(max_size_main_px_f * self.min_gap_main.val);
            } break;
            case TargetBound::Type::RELATIVE_W: {
                return as<int>(as<f32>(padded_max_size_px.x) * self.min_gap_main.val);
            } break;
            case TargetBound::Type::RELATIVE_H: {
                return as<int>(as<f32>(padded_max_size_px.y) * self.min_gap_main.val);
            } break;
            default: {
                // NOLINTNEXTLINE(bugprone-lambda-function-name) not a problem
                MLE_W("Invalid min gap type: {}. Returning 0.", self.min_gap_main.type);
                return 0;
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
        done &= listCalculateSize(main_is_x, max_size_main_px_f, max_size_cross_px_f, root_size_main_px_f, root_size_cross_px_f, cld.state.cross,
                                  cld.size_cross_px, cld.flex.size.cross, cld.target_cross.size, false, cud.size_provider != nullptr, cud.target_aspect_ratio);

        if (done) {
            cld.done = true;
            continue;
        }

        if (cld.state.main == ChildUpdatingData::ListSizeData::CalcState::FLEX) {
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
    f32 flex_share_main = (flex_acc_main > 0.0F) ? as<f32>(remaining_px_main) / flex_acc_main : 0.0F;

    for (auto c : list_children) {
        auto& cld = list_children_data[c];
        if (!cld.valid || cld.done) {
            continue;
        }
        auto& cud = children_updating_data.at(c);

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

        cld.done = cld.state.main == ChildUpdatingData::ListSizeData::CalcState::DONE && cld.state.cross == ChildUpdatingData::ListSizeData::CalcState::DONE &&
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
        } else {
            MLE_ASSERT_LOG(cld.size_main_px > 0 && cld.size_cross_px > 0, "Negative or zero size calculated for list child. main: {}, cross: {}",
                           cld.size_main_px, cld.size_cross_px);

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
}

// enum class ListDirection : u8 { HORIZONTAL, VERTICAL, HORIZONTAL_REVERSED, VERTICAL_REVERSED };
// enum class Justify : u8 { START, CENTER, END, SPACE_BETWEEN, SPACE_AROUND, SPACE_EVENLY };
// enum class AlignCross : u8 { START, CENTER, END };
// enum class WrapMode : u8 { NO, WRAP, WRAP_REVERSE };
// If wrap runs out of cross space, it should stop adding keep adding offscreen
// The same is true for no wrap mode but on main axis

// void calculateListChildrenPosition(const Entt& e, const Container& self, vec2i padded_max_size_px,
//                                    std::map<entt::entity, ChildUpdatingData>& children_updating_data, const std::vector<entt::entity>& list_children) {
//     // const bool main_is_x = self.list_direction == Container::ListDirection::HORIZONTAL || self.list_direction ==
//     // Container::ListDirection::HORIZONTAL_REVERSED; const bool cross_reversed =
//     //     self.list_direction == Container::ListDirection::HORIZONTAL_REVERSED || self.list_direction == Container::ListDirection::VERTICAL_REVERSED;
//     //
//     // int main_axis_size = main_is_x ? padded_max_size_px.x : padded_max_size_px.y;
//
//     // int current_pos_main = 0;
//     // int current_pos_cross = 0;
//
//     // const f32 origin_cross = [&]() {
//     //     switch (self.list_align_cross) {
//     //         case Container::ListAlignCross::START:
//     //             return 0.0F;
//     //         case Container::ListAlignCross::CENTER:
//     //             return 0.5F;
//     //         case Container::ListAlignCross::END:
//     //             return 1.0F;
//     //         default:
//     //             // NOLINTNEXTLINE(bugprone-lambda-function-name) not a problem
//     //             MLE_UNREACHABLE_LOG("Invalid list cross align: {}", (int)self.list_align_cross);
//     //             return 0.0F;
//     //     }
//     // }();
//
//     // for (auto c : list_children) {
//     //     auto& cud = children_updating_data[c];
//     // }
// }

}  // namespace

vec2u Container::calculateChildrenBounds(const Entt& e, vec2u max_size_px) const {
    auto children = o.get();
    auto [flex_children, list_children] = separateFlexListChildren(e.ui(), children);
    auto sorted_by_dependencies = sortChildrenByDependency(e.ui(), flex_children);

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

    calculateListChildrenSize(e, *this, padded_max_size_px, list_children, children_data);

    return accumulateChildrenBounds();
}

vec2u Container::accumulateChildrenBounds() const {
    return {};
}
}  // namespace mle::ui::comp
