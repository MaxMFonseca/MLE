#include "Container.h"

#include "mle/lua/Utils.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Bounds.h"

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
    setAsSizeProvider(e);
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

void Container::setAsSizeProvider(const Entt& e) {
    e.emplaceOrReplace<SizePovider>(sizeProvider);
};

vec2u Container::sizeProvider(const Entt& e, vec2u max_size) {
    auto& self = e.get<Container>();
    self.computeChildrenBounds(e, max_size);
    return self.internal_size;
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

std::pair<std::vector<entt::entity>, std::vector<entt::entity>> separateFlexFromListChildren(UI& ui, std::span<const EntityStorage::Entry> span) {
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

struct UpdatingChildData {
    comp::TargetPosition target_position{};
    comp::TargetSize target_size{};
    comp::TargetMargin target_margin{};
    comp::SizePovider* size_provider = nullptr;
    f32 target_aspect_ratio = 0.0F;
    vec2f target_origin = {0.0F, 0.0F};

    Recti new_bounds_px{};

    struct {
        struct {
            int main_a{}, main_b{}, cross_a{}, cross_b{};
        } margin_px{};

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
        } flex{};

        struct AxisTargets {
            TargetBound size{}, margin_a{}, margin_b{};
        };
        AxisTargets target_main{}, target_cross{};

        int size_main_px = 0;
        int size_cross_px = 0;
        int pos_main_px = 0;
        int pos_cross_px = 0;

    } list_data{};

    explicit UpdatingChildData(const Entt& e) :
        size_provider(e.tryGet<comp::SizePovider>()) {
        if (const auto* target_size = e.tryGet<comp::TargetSize>(); target_size) {
            this->target_size = *target_size;
        }
        if (const auto* target_position = e.tryGet<comp::TargetPosition>(); target_position) {
            this->target_position = *target_position;
        }
        if (const auto* target_margin = e.tryGet<comp::TargetMargin>(); target_margin) {
            this->target_margin = *target_margin;
        }
        if (const auto* aspect_ratio = e.tryGet<comp::TargetAspectRatio>(); aspect_ratio) {
            target_aspect_ratio = aspect_ratio->o;
        }
        if (const auto* origin = e.tryGet<comp::TargetOrigin>(); origin) {
            target_origin = origin->o;
        }
    };

    void gatterListData(bool main_is_x) {
        if (main_is_x) {
            list_data.target_main = {.size = target_size.x, .margin_a = target_margin.l, .margin_b = target_margin.r};
            list_data.target_cross = {.size = target_size.y, .margin_a = target_margin.t, .margin_b = target_margin.b};
        } else {
            list_data.target_main = {.size = target_size.y, .margin_a = target_margin.t, .margin_b = target_margin.b};
            list_data.target_cross = {.size = target_size.x, .margin_a = target_margin.l, .margin_b = target_margin.r};
        }
    }
};

void calculateMargin(const bool main_is_x, const f32 max_size_main_f, const f32 max_size_cross_f, const f32 root_size_main_f, const f32 root_size_cross_f,
                     auto& ret_px, auto& ret_flex, const auto& target_margin, const bool is_main) {
    switch (target_margin.type) {
        case TargetBound::Type::PX: {
            ret_px = as<int>(target_margin.val);
        } break;
        case TargetBound::Type::DEFAULT:
        case TargetBound::Type::RELATIVE: {
            ret_px = is_main ? as<int>(max_size_main_f * target_margin.val) : as<int>(max_size_cross_f * target_margin.val);
        } break;
        case TargetBound::Type::ROOT: {
            ret_px = is_main ? as<int>(root_size_main_f * target_margin.val) : as<int>(root_size_cross_f * target_margin.val);
        } break;
        case TargetBound::Type::RELATIVE_W: {
            f32 max_size_width_f = main_is_x ? max_size_main_f : max_size_cross_f;
            ret_px = as<int>(max_size_width_f * target_margin.val);
        } break;
        case TargetBound::Type::RELATIVE_H: {
            f32 max_size_height_f = main_is_x ? max_size_cross_f : max_size_main_f;
            ret_px = as<int>(max_size_height_f * target_margin.val);
        } break;
        case TargetBound::Type::FLEX_SHARE: {
            ret_flex = target_margin.val;
        } break;
        default: {
            // NOLINTNEXTLINE(bugprone-lambda-function-name) just a log
            MLE_W("Margin cant be {}. Returning 0.", target_margin.type);
            ret_px = 0;
            ret_flex = 0;
        } break;
    };
};

void calculateSize(const bool main_is_x, const f32 max_size_main_f, const f32 max_size_cross_f, const f32 root_size_main_f, const f32 root_size_cross_f,
                   auto& ret_px, auto& ret_flex, auto& fit, auto& ar_defined, const auto& target_size, const bool is_main, const bool has_size_provider,
                   const f32 aspect_ratio) {
    switch (target_size.type) {
        case TargetBound::Type::PX: {
            ret_px = as<int>(target_size.val);
        } break;
        case TargetBound::Type::RELATIVE: {
            ret_px = is_main ? as<int>(max_size_main_f * target_size.val) : as<int>(max_size_cross_f * target_size.val);
        } break;
        case TargetBound::Type::ROOT: {
            ret_px = is_main ? as<int>(root_size_main_f * target_size.val) : as<int>(root_size_cross_f * target_size.val);
        } break;
        case TargetBound::Type::RELATIVE_W: {
            f32 max_size_width_f = main_is_x ? max_size_main_f : max_size_cross_f;
            ret_px = as<int>(max_size_width_f * target_size.val);
        } break;
        case TargetBound::Type::RELATIVE_H: {
            f32 max_size_height_f = main_is_x ? max_size_cross_f : max_size_main_f;
            ret_px = as<int>(max_size_height_f * target_size.val);
        } break;
        case TargetBound::Type::FLEX_SHARE: {
            ret_flex = target_size.val;
        } break;
        case TargetBound::Type::FIT: {
            fit = true;
        } break;
        case TargetBound::Type::DEFAULT: {
            if (has_size_provider) {
                fit = true;
            } else if (aspect_ratio > 0) {
                ret_flex = 1.0F;
            } else {
                ar_defined = true;
            }
        } break;
    };
};

}  // namespace

void Container::computeChildrenBounds(const Entt& e, vec2u max_size) {
    auto& container = e.get<comp::Container>();
    auto children = container.o.get();
    auto [flex_children, list_children] = separateFlexFromListChildren(e.ui(), children);
    auto sorted_by_dependencies = sortChildrenByDependency(e.ui(), flex_children);

    const auto* padding_c = e.tryGet<comp::TargetPadding>();
    PaddingPx padding{};
    if (padding_c) {
        padding = padding_c->calc(e.ui(), max_size);
        max_size.x -= padding.l + padding.r;
        max_size.y -= padding.t + padding.b;
    }

    std::map<entt::entity, UpdatingChildData> children_data;
    for (const auto& c : children) {
        Entt centt{e.ui(), c.e};
        children_data[c.e] = UpdatingChildData{centt};
    }

    // VERTICAL ONLY FOR NOW
    const auto main_is_x = false;
    const u32 max_size_main = max_size.y;
    const u32 max_size_cross = max_size.x;
    const f32 max_size_main_f = as<f32>(max_size_main);
    const f32 max_size_cross_f = as<f32>(max_size_cross);
    const auto root_size = e.ui().getRootSize();
    const f32 root_size_main_f = as<f32>(root_size.x);
    const f32 root_size_cross_f = as<f32>(root_size.y);

    f32 flex_acc_main = 0;
    int px_acc_main = 0;

    for (auto c : list_children) {
        Entt centt{e.ui(), c};

        auto& c_data_g = children_data[c];
        c_data_g.gatterListData(main_is_x);
        auto& cld = c_data_g.list_data;

        calculateMargin(main_is_x, max_size_main_f, max_size_cross_f, root_size_main_f, root_size_cross_f, cld.margin_px.main_b, cld.flex.margin.main_b,
                        cld.target_main.margin_b, true);
        calculateMargin(main_is_x, max_size_main_f, max_size_cross_f, root_size_main_f, root_size_cross_f, cld.margin_px.main_a, cld.flex.margin.main_a,
                        cld.target_main.margin_a, true);
        calculateMargin(main_is_x, max_size_main_f, max_size_cross_f, root_size_main_f, root_size_cross_f, cld.margin_px.cross_b, cld.flex.margin.cross_b,
                        cld.target_cross.margin_b, false);
        calculateMargin(main_is_x, max_size_main_f, max_size_cross_f, root_size_main_f, root_size_cross_f, cld.margin_px.cross_a, cld.flex.margin.cross_a,
                        cld.target_cross.margin_a, false);

        calculateSize(main_is_x, max_size_main_f, max_size_cross_f, root_size_main_f, root_size_cross_f, cld.size_main_px, cld.flex.size.main, cld.is_fit_main,
                      cld.target_main.size, cld.is_main_ar_defined, true, c_data_g.size_provider != nullptr, c_data_g.target_aspect_ratio);
        calculateSize(main_is_x, max_size_main_f, max_size_cross_f, root_size_main_f, root_size_cross_f, cld.size_cross_px, cld.flex.size.cross,
                      cld.is_fit_cross, cld.target_cross.size, cld.is_cross_ar_defined, false, c_data_g.size_provider != nullptr, c_data_g.target_aspect_ratio);

        // if all 3 of cross are flex I can comupte here
        // if both sizes are fit I can compute here
        // if any size is ar defined and the other is set, I can compute here, otherwise wait for flex pass
    }

    // TODO
}
}  // namespace mle::ui::comp
