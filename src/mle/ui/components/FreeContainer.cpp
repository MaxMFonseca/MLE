#include "FreeContainer.h"

#include "mle/lua/Utils.h"
#include "mle/ui/components/ContainerUtils.h"

namespace mle::ui::comp {
void FreeContainer::apply(const Entt& e, const sol::object& obj) {
    auto table_r = lua::tryAs<sol::table>(obj);
    if (!table_r) {
        MLE_E("Invalid object to apply Container component at entity {}. Expected table.", e.name());
        return;
    }
    e.patchOrEmplace<FreeContainer>([&](FreeContainer& c) { c.set(*table_r); });
}

void FreeContainer::on_construct(entt::registry& registry, const entt::entity entt) {
    if (registry.any_of<SizeProvider>(entt)) {
        MLE_W("An element shouldnt have multiple SizeProvider components.");
        return;
    }
    registry.emplace<SizeProvider>(entt, [](const Entt& e, vec2u max_size) { return e.get<comp::FreeContainer>().calculateChildrenBounds(e, max_size); });
}

void FreeContainer::on_destroy(entt::registry& registry, const entt::entity entt) {
    registry.remove<SizeProvider>(entt);
};

void FreeContainer::set(const sol::table& table) {
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
}

namespace {
struct FreeCalculator {
    // NOLINTNEXTLINE(readability-function-cognitive-complexity) Not that complex
    FreeCalculator(Entt free_ew, [[maybe_unused]] const FreeContainer& free_container, vec2i padded_size, const std::vector<entt::entity>& free_children,
                   PaddingPx padding_px, std::map<entt::entity, ChildBoundsCalcData>& cbcds) {
        auto sorted_by_dependencies = sortByDependency(free_ew.ui(), free_children);

        const auto root_size = free_ew.ui().getRootMaxSize();
        const auto root_size_f = vec2f{as<f32>(root_size.x), as<f32>(root_size.y)};

        std::unordered_map<entt::entity, Recti> children_full_rect;
        children_full_rect.reserve(free_children.size());

        for (auto c : sorted_by_dependencies) {
            Entt centt{free_ew.ui(), c};
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
                if (!centt.getRelationship().isChildOf(free_ew.e())) {
                    MLE_W("Child {} is not a child of the container. Ignoring x dependency.", centt.name());
                } else {
                    const auto dep_entt = Entt{free_ew.ui(), cbcd.target.position.xdep.e};
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
                if (!centt.getRelationship().isChildOf(free_ew.e())) {
                    MLE_W("Child {} is not a child of the container. Ignoring y dependency.", centt.name());
                } else {
                    const auto dep_entt = Entt{free_ew.ui(), cbcd.target.position.ydep.e};
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
                            cbcd.new_size.x = as<int>(as<f32>(padded_size.x));
                        }
                    }
                } break;
                default: {
                    MLE_E("Weird enum value for target size x type: {}.", (int)cbcd.target.size.x.type);
                    continue;
                } break;
            }

            if (cbcd.target.size.xdep.e != entt::null) {
                if (!centt.getRelationship().isChildOf(free_ew.e())) {
                    MLE_W("Child {} is not a child of the container. Ignoring x size dependency.", centt.name());
                } else {
                    const auto dep_entt = Entt{free_ew.ui(), cbcd.target.size.xdep.e};
                    const auto tb = cbcd.target.size.xdep.dep_tb;
                    Recti dep_rect;
                    auto& dep_bounds = dep_entt.get<comp::Bounds>();
                    dep_rect = dep_bounds.parent_px;
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
                } break;
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
                            cbcd.new_size.y = as<int>(as<f32>(padded_size.y));
                        }
                    }
                } break;
                default: {
                    MLE_E("Weird enum value for target size y type: {}.", (int)cbcd.target.size.y.type);
                    continue;
                } break;
            }

            if (cbcd.target.size.ydep.e != entt::null) {
                if (!centt.getRelationship().isChildOf(free_ew.e())) {
                    MLE_W("Child {} is not a child of the container. Ignoring y size dependency.", centt.name());
                } else {
                    const auto dep_entt = Entt{free_ew.ui(), cbcd.target.size.ydep.e};
                    const auto tb = cbcd.target.size.ydep.dep_tb;
                    Recti dep_rect;
                    auto& dep_bounds = dep_entt.get<comp::Bounds>();
                    dep_rect = dep_bounds.parent_px;
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
                    fit_max_size_px.x = padded_size.x;
                }
                if (fit_max_size_px.y <= 0) {
                    fit_max_size_px.y = padded_size.y;
                }

                vec2u size_from_provider = cbcd.size_provider->call(centt, fit_max_size_px);

                if (x_is_fit) {
                    cbcd.new_size.x = as<int>(size_from_provider.x);
                }
                if (y_is_fit) {
                    cbcd.new_size.y = as<int>(size_from_provider.y);
                }
            }

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
}  // namespace

[[nodiscard]] vec2u FreeContainer::calculateChildrenBounds(const Entt& e, vec2u max_size) const {
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

    FreeCalculator free_calculator{e, *this, padded_max_size, children, padding_result, cbcds};

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
