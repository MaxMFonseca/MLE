#include "ListLayout.h"

#include "mle/common/Assert.h"
#include "mle/lua/Utils.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/Base.h"
#include "mle/ui/element/Container.h"

namespace mle::ui::element {
void ListLayout::lkhList(entt::entity self, const sol::object& obj) {
    MLE_T("list");
    MLE_ASSERT(lua::valid<sol::table>(obj));
    auto& reg = getRegistry();
    MLE_ASSERT_LOG(!reg.try_get<comp::Container>(self), "ListLayout should not be applied to an entity that already has a Container component");

    sol::table table = obj.as<sol::table>();

    auto list_layout = std::make_unique<ListLayout>();

    if (const auto layout_r = table["layout"]; layout_r.valid()) {
        if (layout_r.is<std::string>()) {
            auto layout = layout_r.get<std::string>();
            if (layout == "rows" || layout == "vert" || layout == "vertical" || layout == "v" || layout == "V" || layout == "r" || layout == "R") {
                list_layout->axis = Axis::Y;
            } else if (layout == "cols" || layout == "horizontal" || layout == "h" || layout == "H" || layout == "c" || layout == "C") {
                list_layout->axis = Axis::X;
            }
        } else {
            MLE_UNREACHABLE_LOG("Unexpected layout type: {}", layout_r.get_type());
        }
    }

    // FIXME: this should be TargetBound
    if (const auto child_gap_r = table["child_gap"]; child_gap_r.valid()) {
        if (child_gap_r.is<int>()) {
            list_layout->child_gap = child_gap_r.get<int>();
        }
    }

    comp::Container::add(self, std::move(list_layout), table);
}

void ListLayout::updateChildrenBounds(entt::entity self, Recti context, bool force_update) const {
    if (axis == Axis::X) {
        updateChildrenBoundsX(self, context, force_update);
    } else {
        updateChildrenBoundsY(self, context, force_update);
    }
}

void ListLayout::updateChildrenBoundsX([[maybe_unused]] entt::entity self, [[maybe_unused]] Recti context, [[maybe_unused]] bool force_update) const {
    MLE_TODO;
}

namespace {
struct CalcMainAxisData {
    const comp::TargetBound& tb;
    f32& flex_shares;
    int& out_accumulate;
};
void calcMainAxis(const CalcMainAxisData& v) {
    switch (v.tb.type) {
        case comp::TargetBound::Type::DEFAULT:
        case comp::TargetBound::Type::PX: {
            v.out_accumulate += as<int>(v.tb.val);
        } break;
        case comp::TargetBound::Type::FLEX_SHARE: {
            v.flex_shares += v.tb.val;
        } break;
        default:
            MLE_UNREACHABLE_LOG("Unexpected type: {}", v.tb.type);
    }
}
}  // namespace

void ListLayout::updateChildrenBoundsY(entt::entity self, Recti context, [[maybe_unused]] bool force_update) const {
    auto& reg = getRegistry();
    auto& container = reg.get<comp::Container>(self);
    auto children = container.getChildren();

    std::vector<comp::Container::ChildBuildInfo> cinfos;
    cinfos.reserve(children.size());

    auto* target_padding = reg.try_get<comp::TargetPadding>(self);

    int height = 0;

    f32 flex_shares = 0.0F;
    for (auto c : children) {
        auto& cinfo = cinfos.emplace_back(reg, c);
        if (cinfo.bounds.immutable) {
            continue;
        }
        cinfo.bounds.bounds = {};

        int m_t = 0, m_b = 0, content = 0;

        if (cinfo.target_margin) {
            calcMainAxis({
                .tb = cinfo.target_margin->t,
                .flex_shares = flex_shares,
                .out_accumulate = m_t,
            });
            calcMainAxis({
                .tb = cinfo.target_margin->b,
                .flex_shares = flex_shares,
                .out_accumulate = m_b,
            });
        };

        if (cinfo.target_size) {
            calcMainAxis({
                .tb = cinfo.target_size->y,
                .flex_shares = flex_shares,
                .out_accumulate = content,
            });
        }

        cinfo.bounds.bounds.size.y = content;
        cinfo.margin[as<int>(BoxFace::T)] = m_t;
        cinfo.margin[as<int>(BoxFace::B)] = m_b;
        height += content + m_t + m_b;
        height += as<int>(child_gap);
    }

    f32 share_height = 0.0F;
    int remaining_space = context.height() - height;
    if (remaining_space < 0) {
        MLE_TODO;
        // TODO: make scrollable and clip
    } else if (remaining_space != 0) {
        share_height = as<f32>(remaining_space) / flex_shares;
    }

    int top = context.top();
    int left = context.left();
    if (target_padding) {
        switch (target_padding->t.type) {
            case comp::TargetBound::Type::DEFAULT:
            case comp::TargetBound::Type::PX: {
                top += as<int>(target_padding->t.val);
            } break;
            default:
                MLE_UNREACHABLE_LOG("Unexpected target padding type: {}", target_padding->t.type);
        }

        switch (target_padding->l.type) {
            case comp::TargetBound::Type::DEFAULT:
            case comp::TargetBound::Type::PX: {
                left += as<int>(target_padding->l.val);
            } break;
            default:
                MLE_UNREACHABLE_LOG("Unexpected target padding type: {}", target_padding->l.type);
        }
    }
    MLE_VC(top);
    for (usize i = 0; i < children.size(); i++) {  // NOLINT
        auto& cinfo = cinfos[i];
        if (cinfo.bounds.immutable) {
            continue;
        }

        int flex_m_t = 0, flex_m_b = 0, flex_content = 0;

        if (cinfo.target_margin) {
            flex_m_t = cinfo.target_margin->t.type == comp::TargetBound::Type::FLEX_SHARE ? as<int>(cinfo.target_margin->t.val * share_height) : 0;
            flex_m_b = cinfo.target_margin->b.type == comp::TargetBound::Type::FLEX_SHARE ? as<int>(cinfo.target_margin->b.val * share_height) : 0;
        }
        if (cinfo.target_size) {
            flex_content = cinfo.target_size->y.type == comp::TargetBound::Type::FLEX_SHARE ? as<int>(cinfo.target_size->y.val * share_height) : 0;
        }

        top += flex_m_t + cinfo.margin[as<int>(BoxFace::T)];
        cinfo.bounds.bounds.pos.y = top;
        cinfo.bounds.bounds.size.y += flex_content;
        top += cinfo.bounds.bounds.size.y;
        top += flex_m_b + cinfo.margin[as<int>(BoxFace::B)];
        top += child_gap;

        // TODO: this
        cinfo.bounds.bounds.size.x = as<int>(cinfo.target_size->x.val);
        cinfo.bounds.bounds.pos.x = left;

        MLE_VC(cinfo.bounds.bounds);
    }
}
}  // namespace mle::ui::element
