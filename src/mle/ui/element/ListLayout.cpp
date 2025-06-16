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
            auto layout = toLower(layout_r.get<std::string>());
            if (layout == "rows" || layout == "vert" || layout == "vertical" || layout == "v" || layout == "r" || layout == "y") {
                list_layout->axis = Axis::Y;
            } else if (layout == "cols" || layout == "horizontal" || layout == "h" || layout == "c" || layout == "x") {
                list_layout->axis = Axis::X;
            } else {
                MLE_UNREACHABLE_LOG("Unexpected layout string: {}", layout);
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

void ListLayout::updateChildrenBounds(entt::entity self, Recti content_rect, bool force_update) const {
    if (axis == Axis::X) {
        updateChildrenBoundsX(self, content_rect, force_update);
    } else {
        updateChildrenBoundsY(self, content_rect, force_update);
    }
}

void ListLayout::updateChildrenBoundsX(entt::entity /*self*/, Recti /*content_rect*/, bool /*force_update*/) const {
    MLE_TODO;
}

namespace {
void updateChildrenBoundsYcalcOffSide(ListLayout::ChildBuildInfo& cinfo, Recti content_rect) {
    f32 off_flex_shares = cinfo.content_flex.x + cinfo.margin_flex.r + cinfo.margin_flex.l;
    f32 off_remaining = as<f32>(content_rect.width() - cinfo.margin_px.r - cinfo.margin_px.l - cinfo.content_px.x);
    f32 off_flex_value_px = off_flex_shares != 0.0F && off_remaining != 0.0F ? off_remaining / std::max(1.0F, off_flex_shares) : 0.0F;

    cinfo.bounds.bounds.pos.x = content_rect.left() + cinfo.margin_px.l;
    cinfo.bounds.bounds.pos.x += as<int>(cinfo.margin_flex.l * off_flex_value_px);

    cinfo.bounds.bounds.size.x = cinfo.content_px.x + as<int>(cinfo.content_flex.x * off_flex_value_px);
}
}  // namespace

void ListLayout::updateChildrenBoundsY(entt::entity self, Recti content_rect, [[maybe_unused]] bool force_update) const {
    auto& reg = getRegistry();
    auto& container = reg.get<comp::Container>(self);
    auto children = container.getChildren();

    std::vector<ChildBuildInfo> cinfos;
    cinfos.reserve(children.size());

    comp::TargetPadding::addToRect(self, content_rect);
    MLE_VC(content_rect);

    int main_axis_size = 0;
    f32 main_axis_flex_shares = 0.0F;
    int non_immutable_children_cound = 0;

    for (auto c : children) {
        auto& cinfo = cinfos.emplace_back(reg, c, content_rect);
        if (cinfo.bounds.immutable) {
            continue;
        }

        bool main_is_default = cinfo.content_flex.y == 0.0F && cinfo.content_px.y == 0;
        bool off_is_default = cinfo.content_flex.x == 0.0F && cinfo.content_px.x == 0;

        if (!off_is_default) {
            updateChildrenBoundsYcalcOffSide(cinfo, content_rect);
        }

        if (main_is_default) {
            if (off_is_default || cinfo.aspect_ratio == 0.0F) {
                cinfo.content_flex.y = 1.0F;
                main_is_default = false;
            } else {
                cinfo.content_px.y = as<int>(as<f32>(cinfo.bounds.bounds.size.x) / cinfo.aspect_ratio);
            }
        }

        main_axis_size += cinfo.content_px.y + cinfo.margin_px.t + cinfo.margin_px.b;
        main_axis_flex_shares += cinfo.content_flex.y + cinfo.margin_flex.t + cinfo.margin_flex.b;
        non_immutable_children_cound++;
    }

    main_axis_size += (non_immutable_children_cound - 1) * child_gap;

    f32 main_axis_remaining_px = as<f32>(content_rect.height() - main_axis_size);
    f32 main_axis_share_value_px = 0.0F;
    if (main_axis_remaining_px < 0) {
        MLE_TODO;
    } else if (main_axis_remaining_px != 0 && main_axis_flex_shares != 0.0F) {
        main_axis_share_value_px = as<f32>(main_axis_remaining_px) / std::max(1.0F, main_axis_flex_shares);
    }

    int main_axis_pos = content_rect.top();
    for (auto& cinfo : cinfos) {
        if (cinfo.bounds.immutable) {
            continue;
        }

        cinfo.margin_px.t += as<int>(cinfo.margin_flex.t * main_axis_share_value_px);
        cinfo.margin_px.b += as<int>(cinfo.margin_flex.b * main_axis_share_value_px);
        cinfo.content_px.y += as<int>(cinfo.content_flex.y * main_axis_share_value_px);

        cinfo.bounds.bounds.pos.y = main_axis_pos + cinfo.margin_px.t;
        cinfo.bounds.bounds.size.y = cinfo.content_px.y;

        main_axis_pos = cinfo.bounds.bounds.pos.y + cinfo.bounds.bounds.size.y + cinfo.margin_px.b + child_gap;

        if (cinfo.bounds.bounds.size.x == 0) {
            if (cinfo.aspect_ratio != 0) {
                cinfo.content_px.x = as<int>(as<f32>(cinfo.bounds.bounds.size.y) * cinfo.aspect_ratio);
            } else {
                cinfo.content_flex.x = 1.0F;
            }

            updateChildrenBoundsYcalcOffSide(cinfo, content_rect);
        }
    }
}

ListLayout::ChildBuildInfo::ChildBuildInfo(entt::registry& r, entt::entity e, Recti parent_rect) :
    bounds(r.get<comp::Bounds>(e)),
    target_size(r.try_get<comp::TargetSize>(e)),
    target_position(r.try_get<comp::TargetPosition>(e)),
    target_margin(r.try_get<comp::TargetMargin>(e))
// target_padding(r.try_get<comp::TargetPadding>(e)),
// origin(r.try_get<comp::Origin>(e)),
{
    using BType = comp::TargetBound::Type;

    if (bounds.immutable) {
        return;
    }

    bounds.bounds = {};

    if (target_size) {
        switch (target_size->x.type) {
            case BType::DEFAULT:
            case BType::PX: {
                content_px.x = as<int>(target_size->x.val);
            } break;
            case BType::PARENT: {
                content_px.x = as<int>(target_size->x.val * as<f32>(parent_rect.size.x));
            } break;
            case BType::FLEX_SHARE: {
                content_flex.x = target_size->x.val;
            } break;
            case BType::SELF_H:
            case BType::SELF: {
                aspect_ratio = target_size->x.val;
            } break;
            default: {
                MLE_UNREACHABLE_LOG("Unexpected target size type: {}", target_size->x.type);
            }
        }
        switch (target_size->y.type) {
            case BType::DEFAULT:
            case BType::PX: {
                content_px.y = as<int>(target_size->y.val);
            } break;
            case BType::PARENT: {
                content_px.y = as<int>(target_size->y.val * as<f32>(parent_rect.size.y));
            } break;
            case BType::FLEX_SHARE: {
                content_flex.y = target_size->y.val;
            } break;
            case BType::SELF_W:
            case BType::SELF: {
                aspect_ratio = 1.0F / target_size->y.val;
            } break;
            default: {
                MLE_UNREACHABLE_LOG("Unexpected target size type: {}", target_size->y.type);
            }
        }
    }
    if (target_margin) {
        switch (target_margin->t.type) {
            case BType::DEFAULT:
            case BType::PX: {
                margin_px.t = as<int>(target_margin->t.val);
            } break;
            case BType::PARENT: {
                margin_px.t = as<int>(target_margin->t.val * as<f32>(parent_rect.size.y));
            } break;
            case BType::FLEX_SHARE: {
                margin_flex.t = target_margin->t.val;
            } break;
            default: {
                MLE_UNREACHABLE_LOG("Unexpected target margin type: {}", target_margin->t.type);
            }
        }
        switch (target_margin->b.type) {
            case BType::DEFAULT:
            case BType::PX: {
                margin_px.b = as<int>(target_margin->b.val);
            } break;
            case BType::PARENT: {
                margin_px.b = as<int>(target_margin->b.val * as<f32>(parent_rect.size.y));
            } break;
            case BType::FLEX_SHARE: {
                margin_flex.b = target_margin->b.val;
            } break;
            default: {
                MLE_UNREACHABLE_LOG("Unexpected target margin type: {}", target_margin->b.type);
            }
        }
        switch (target_margin->l.type) {
            case BType::DEFAULT:
            case BType::PX: {
                margin_px.l = as<int>(target_margin->l.val);
            } break;
            case BType::PARENT: {
                margin_px.l = as<int>(target_margin->l.val * as<f32>(parent_rect.size.x));
            } break;
            case BType::FLEX_SHARE: {
                margin_flex.l = target_margin->l.val;
            } break;
            default: {
                MLE_UNREACHABLE_LOG("Unexpected target margin type: {}", target_margin->l.type);
            }
        }
        switch (target_margin->r.type) {
            case BType::DEFAULT:
            case BType::PX: {
                margin_px.r = as<int>(target_margin->r.val);
            } break;
            case BType::PARENT: {
                margin_px.r = as<int>(target_margin->r.val * as<f32>(parent_rect.size.x));
            } break;
            case BType::FLEX_SHARE: {
                margin_flex.r = target_margin->r.val;
            } break;
            default: {
                MLE_UNREACHABLE_LOG("Unexpected target margin type: {}", target_margin->r.type);
            }
        }
    }

    if (const auto* target_aspect_ratio = r.try_get<comp::TargetAspectRatio>(e); target_aspect_ratio) {
        aspect_ratio = target_aspect_ratio->v;
    }
}
}  // namespace mle::ui::element
