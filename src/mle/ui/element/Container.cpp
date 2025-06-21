#include "Container.h"

#include "mle/common/Assert.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/Base.h"
#include "mle/ui/element/Renderable.h"
#include "sol/function_types_templated.hpp"

namespace mle::ui::element::comp {
namespace {
struct ChildBuildInfo {
    ChildBuildInfo(entt::registry& r, entt::entity e, Recti parent_rect);

    comp::Bounds& bounds;
    const comp::TargetSize* target_size;
    const comp::TargetPosition* target_position;
    const comp::TargetMargin* target_margin;
    const comp::TargetRelations* target_relations;
    const comp::Origin* origin;

    struct {
        int t{}, b{}, l{}, r{};
    } margin_px;
    struct {
        f32 t{}, b{}, l{}, r{};
    } margin_flex;
    vec2i content_px{0};
    vec2f content_flex{0};
    f32 aspect_ratio = 0.0F;
};

ChildBuildInfo::ChildBuildInfo(entt::registry& r, entt::entity e, Recti parent_rect) :
    bounds(r.get<comp::Bounds>(e)),
    target_size(bounds.immutable ? nullptr : r.try_get<comp::TargetSize>(e)),
    target_position(bounds.immutable ? nullptr : r.try_get<comp::TargetPosition>(e)),
    target_margin(bounds.immutable ? nullptr : r.try_get<comp::TargetMargin>(e)),
    target_relations(bounds.immutable ? nullptr : r.try_get<comp::TargetRelations>(e)),
    origin(bounds.immutable ? nullptr : r.try_get<comp::Origin>(e)) {
    //
    using BType = comp::TargetBound::Type;

    if (bounds.immutable) {
        return;
    }

    bounds.bounds = {};

    if (const auto* target_aspect_ratio = r.try_get<comp::TargetAspectRatio>(e); target_aspect_ratio) {
        aspect_ratio = target_aspect_ratio->v;
    } else if (const comp::Renderable* renderable = r.try_get<comp::Renderable>(e); renderable) {
        aspect_ratio = renderable->getAspectRatio();
    }

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
                MLE_ASSERT(target_size->y.val != 0.0F);
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
}

void updateChildrenBoundsColCross(ChildBuildInfo& cinfo, Recti content_rect) {
    f32 off_flex_shares = cinfo.content_flex.x + cinfo.margin_flex.r + cinfo.margin_flex.l;
    f32 off_remaining = as<f32>(content_rect.width() - cinfo.margin_px.r - cinfo.margin_px.l - cinfo.content_px.x);
    f32 off_flex_value_px = off_flex_shares != 0.0F && off_remaining != 0.0F ? off_remaining / std::max(1.0F, off_flex_shares) : 0.0F;

    cinfo.bounds.bounds.pos.x = content_rect.left() + cinfo.margin_px.l;
    cinfo.bounds.bounds.pos.x += as<int>(cinfo.margin_flex.l * off_flex_value_px);

    cinfo.bounds.bounds.size.x = cinfo.content_px.x + as<int>(cinfo.content_flex.x * off_flex_value_px);
}

}  // namespace

void Container::lkh(entt::entity self, const sol::object& obj) {
    MLE_T("Container");

    auto& reg = getRegistry();

    auto* comp = reg.try_get<Container>(self);
    if (!comp) {
        comp = &reg.emplace<Container>(self);
    }

    comp::Renderable::add(self, [](entt::entity self) -> RenderableInterface& { return getRegistry().get<Container>(self); });

    comp->update(self, obj);
}

void Container::update(entt::entity self, const sol::object& obj) {
    auto table = lua::as<sol::table>(obj);

    if (const auto direction_r = table["direction"]; direction_r.valid()) {
        MLE_ASSERT(direction_r.is<std::string>());
        auto layout = toLower(direction_r.get<std::string>());
        if (layout == "row") {
            direction_ = Direction::ROW;
        } else if (layout == "row_r") {
            direction_ = Direction::ROW_R;
        } else if (layout == "col") {
            direction_ = Direction::COL;
        } else if (layout == "col_r") {
            direction_ = Direction::COL_R;
        } else {
            MLE_UNREACHABLE_LOG("Unexpected layout string: {}", layout);
        }
    }

    if (const auto child_gap_r = table["gap"]; child_gap_r.valid()) {
        if (child_gap_r.is<int>()) {
            gap_ = child_gap_r.get<int>();
        }
    }

    addChildren(self, table);
}

entt::entity Container::getChild(const std::string& name) const {
    auto children = getChildren();
    auto it = std::ranges::find_if(children, [name](entt::entity e) {
        auto* namec = getRegistry().try_get<Name>(e);
        return namec && namec->name == name;
    });
    if (it == children.end()) {
        MLE_W("Container: No child with name '{}'", name);
        return entt::null;
    }
    return *it;
}

usize Container::getChildIdx(entt::entity child) const {
    auto children = getChildren();
    auto it = std::ranges::find(children, child);
    if (it == children.end()) {
        MLE_W("Container: No child with entity {}", child);
        return max<usize>();
    }
    return std::distance(children.begin(), it);
}

void Container::addChildren(entt::entity self, const sol::table& table) {
    for (const auto& [key, val] : table) {
        if (key.is<std::string>()) {
            continue;
        }

        addChild(self, val);
    }
}

void Container::addChild(entt::entity self, const sol::table& table, [[maybe_unused]] usize pos) {
    auto& reg = getRegistry();
    auto child_entt = reg.create();
    applyTable(child_entt, table, self);

    children_.add(child_entt, pos);
}

void Container::updateChildrenBounds(entt::entity self, Recti context, bool verify_all, bool force_update) {  // NOLINT
    auto& bounds = getRegistry().get<comp::Bounds>(self).bounds;

    if (!isFitX(self)) {
        context.pos.x = bounds.left();
        context.size.x = bounds.width();
    }
    if (!isFitY(self)) {
        context.pos.y = bounds.top();
        context.size.y = bounds.height();
    }

    // TODO: force
    if (!changed_bounds_children_.empty()) {
        switch (direction_) {
            case Direction::ROW:
                updateChildrenBoundsRow(self, context);
                break;
            case Direction::ROW_R:
                updateChildrenBoundsRowR(self, context);
                break;
            case Direction::COL:
                updateChildrenBoundsCol(self, context);
                break;
            case Direction::COL_R:
                updateChildrenBoundsColR(self, context);
                break;
        }
    }

    if (verify_all) {
        for (auto c : children_.get()) {
            auto* child_container = getRegistry().try_get<Container>(c);
            if (child_container) {
                child_container->updateChildrenBounds(c, context, verify_all, force_update);
            }
        }
    }
}

void Container::notifyChildChangedBounds(entt::entity child) {  // NOLINT
    notifyChildChangedBounds(getRegistry().get<Parent>(child).parent, child);
}

void Container::notifyChildChangedBounds(entt::entity self, entt::entity child) {  // NOLINT
    auto& reg = getRegistry();
    auto& self_container = reg.get<Container>(self);
    self_container.changed_bounds_children_.emplace(child);

    if (isFit(self)) {
        notifyChildChangedBounds(self);
    } else {
        reg.emplace_or_replace<ChildChangedBounds>(self);  // Only mark non fit containers with this so full tree traversal may not be needed
    }
}

void Container::renderComp(const RenderContext& ctx) const {
    auto& reg = getRegistry();
    auto children = children_.get();

    RenderContext child_ctx = ctx;
    child_ctx.parent = ctx.self;

    for (auto child : children) {
        child_ctx.self = child;

        const auto* child_bounds = reg.try_get<Bounds>(child);
        if (!child_bounds) {
            continue;
        }

        Rectf viewport = child_bounds->bounds.asF32();
        viewport.pos += ctx.current_root_image_bounds.pos;

        const auto* bg = reg.try_get<Background>(child);
        const auto* renderable = reg.try_get<Renderable>(child);

        if (bg || renderable) {
            ctx.thread->setViewport(viewport);
            // TODO: scissor
        }

        if (bg) {
            bg->render(child_ctx);
        }
        if (renderable) {
            renderable->render(child_ctx);
        }
    }
}

void Container::updateChildrenBoundsColR(entt::entity /*self*/, Recti /*context*/) {  // NOLINT
    MLE_TODO;
}

void Container::updateChildrenBoundsRow(entt::entity /*self*/, Recti /*context*/) {  // NOLINT
    MLE_TODO;
}

void Container::updateChildrenBoundsRowR(entt::entity /*self*/, Recti /*context*/) {  // NOLINT
    MLE_TODO;
}

void Container::updateChildrenBoundsCol(entt::entity self, Recti content_rect) {  // NOLINT
    auto& reg = getRegistry();
    auto& container = reg.get<comp::Container>(self);
    auto children = container.getChildren();

    std::vector<ChildBuildInfo> cinfos;
    cinfos.reserve(children.size());

    comp::TargetPadding::addToRect(self, content_rect);

    bool update_flex = false;

    std::set<entt::entity> changed_entities;

    // Create child build infos and check if need update flex if any flex changed bounds
    for (auto c : children) {
        auto& cinfo = cinfos.emplace_back(reg, c, content_rect);
        if (!cinfo.target_position && changed_bounds_children_.contains(c)) {
            update_flex = true;
        }
    }

    // Update flex if needed
    if (update_flex) {
        int main_axis_size = 0;
        f32 main_axis_flex_shares = 0.0F;
        int non_immutable_children_cound = 0;

        for (usize i = 0; i < children.size(); i++) {
            auto& cinfo = cinfos[i];
            if (cinfo.bounds.immutable || cinfo.target_position || cinfo.target_relations) {
                continue;
            }

            bool main_is_default = cinfo.content_flex.y == 0.0F && cinfo.content_px.y == 0;
            bool off_is_default = cinfo.content_flex.x == 0.0F && cinfo.content_px.x == 0;

            if (!off_is_default) {
                updateChildrenBoundsColCross(cinfo, content_rect);
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

        main_axis_size += (non_immutable_children_cound - 1) * gap_;

        f32 main_axis_remaining_px = as<f32>(content_rect.height() - main_axis_size);
        f32 main_axis_share_value_px = 0.0F;
        if (main_axis_remaining_px < 0) {
            MLE_TODO;
        } else if (main_axis_remaining_px != 0 && main_axis_flex_shares != 0.0F) {
            main_axis_share_value_px = as<f32>(main_axis_remaining_px) / std::max(1.0F, main_axis_flex_shares);
        }

        int main_axis_pos = content_rect.top();
        for (usize i = 0; i < children.size(); i++) {
            auto& cinfo = cinfos[i];

            if (cinfo.bounds.immutable || cinfo.target_position || cinfo.target_relations) {
                continue;
            }

            cinfo.margin_px.t += as<int>(cinfo.margin_flex.t * main_axis_share_value_px);
            cinfo.margin_px.b += as<int>(cinfo.margin_flex.b * main_axis_share_value_px);
            cinfo.content_px.y += as<int>(cinfo.content_flex.y * main_axis_share_value_px);

            cinfo.bounds.bounds.pos.y = main_axis_pos + cinfo.margin_px.t;
            cinfo.bounds.bounds.size.y = cinfo.content_px.y;

            main_axis_pos = cinfo.bounds.bounds.pos.y + cinfo.bounds.bounds.size.y + cinfo.margin_px.b + gap_;

            if (cinfo.bounds.bounds.size.x == 0) {
                if (cinfo.aspect_ratio != 0) {
                    cinfo.content_px.x = as<int>(as<f32>(cinfo.bounds.bounds.size.y) * cinfo.aspect_ratio);
                } else {
                    cinfo.content_flex.x = 1.0F;
                }

                updateChildrenBoundsColCross(cinfo, content_rect);
            }

            if (cinfo.origin) {
                cinfo.bounds.bounds.pos.x -= as<int>(cinfo.origin->origin.x * as<f32>(cinfo.bounds.bounds.size.x));
                cinfo.bounds.bounds.pos.y -= as<int>(cinfo.origin->origin.y * as<f32>(cinfo.bounds.bounds.size.y));
            }

            changed_entities.emplace(children[i]);
        }
    }

    // Update non flex
    for (usize i = 0; i < children.size(); i++) {
        auto& cinfo = cinfos.at(i);

        if (cinfo.bounds.immutable || cinfo.bounds.bounds.size.x != 0) {
            continue;
        }

        bool need_update = changed_bounds_children_.contains(children[i]);
        if (cinfo.target_relations) {
            for (auto v : cinfo.target_relations->v) {
                need_update |= changed_bounds_children_.contains(v.e);
            }
        }
        if (!need_update) {
            continue;
        }

        if (cinfo.target_relations) {
            for (const auto& r : cinfo.target_relations->v) {
                auto eidx = getChildIdx(r.e);
                MLE_ASSERT(eidx != max<usize>() && eidx < i);
                auto& target_bounds = cinfos[eidx].bounds.bounds;
                if (r.type == TargetRelations::Dep::Type::SIZE_X) {
                    cinfo.bounds.bounds.size.x = as<int>(r.val) * target_bounds.size.x;
                } else if (r.type == TargetRelations::Dep::Type::SIZE_Y) {
                    cinfo.bounds.bounds.size.y = as<int>(r.val) * target_bounds.size.y;
                } else if (r.type == TargetRelations::Dep::Type::POS_X) {
                    cinfo.bounds.bounds.pos.x = target_bounds.pos.x + as<int>(r.val) * target_bounds.size.x;
                } else if (r.type == TargetRelations::Dep::Type::POS_Y) {
                    cinfo.bounds.bounds.pos.y = target_bounds.pos.y + as<int>(r.val) * target_bounds.size.y;
                }
            }
        }

        if (cinfo.target_size) {
            switch (cinfo.target_size->x.type) {
                using Type = TargetBound::Type;
                case Type::DEFAULT:
                case Type::PX: {
                    cinfo.bounds.bounds.size.x += as<int>(cinfo.target_size->x.val);
                } break;
                case Type::PARENT: {
                    cinfo.bounds.bounds.size.x += as<int>(cinfo.target_size->x.val * as<f32>(content_rect.width()));
                } break;
                case Type::FIT: {
                    MLE_ASSERT(cinfo.target_position->x.type == Type::DEFAULT || cinfo.target_position->x.type == Type::PX);
                    cinfo.bounds.bounds.size.x += content_rect.width() - as<int>(cinfo.target_position->x.val);
                } break;
                case Type::SELF_H:
                case Type::SELF:
                    break;
                default: {
                    MLE_UNREACHABLE_LOG("Unexpected target size type: {}", cinfo.target_size->x.type);
                }
            }

            switch (cinfo.target_size->y.type) {
                using Type = TargetBound::Type;
                case Type::DEFAULT:
                case Type::PX: {
                    cinfo.bounds.bounds.size.y += as<int>(cinfo.target_size->y.val);
                } break;
                case Type::PARENT: {
                    cinfo.bounds.bounds.size.y += as<int>(cinfo.target_size->y.val * as<f32>(content_rect.height()));
                } break;
                case Type::FIT: {
                    MLE_ASSERT(cinfo.target_position->y.type == Type::DEFAULT || cinfo.target_position->y.type == Type::PX);
                    cinfo.bounds.bounds.size.y += content_rect.height() - as<int>(cinfo.target_position->y.val);
                } break;
                case Type::SELF_W:
                case Type::SELF:
                    break;
                default: {
                    MLE_UNREACHABLE_LOG("Unexpected target size type: {}", cinfo.target_size->y.type);
                }
            }
        }

        if (cinfo.bounds.bounds.size.x == 0 && cinfo.aspect_ratio != 0) {
            cinfo.bounds.bounds.size.x = as<int>(as<f32>(cinfo.bounds.bounds.size.y) * cinfo.aspect_ratio);
        } else if (cinfo.bounds.bounds.size.y == 0 && cinfo.aspect_ratio != 0) {
            cinfo.bounds.bounds.size.y = as<int>(as<f32>(cinfo.bounds.bounds.size.x) / cinfo.aspect_ratio);
        }

        if (cinfo.bounds.bounds.size.x == 0 || cinfo.bounds.bounds.size.y == 0) {
            auto* renderable = reg.try_get<comp::Renderable>(children[i]);
            if (!renderable) {
                MLE_TODO;
            }
            cinfo.bounds.bounds.size = renderable->getSize();
        }

        MLE_ASSERT(cinfo.bounds.bounds.size.x >= 0 && cinfo.bounds.bounds.size.y >= 0);

        if (cinfo.target_position) {
            switch (cinfo.target_position->x.type) {
                case TargetBound::Type::DEFAULT:
                case TargetBound::Type::PX: {
                    cinfo.bounds.bounds.pos.x += as<int>(cinfo.target_position->x.val);
                } break;
                case TargetBound::Type::PARENT: {
                    cinfo.bounds.bounds.pos.x += as<int>(cinfo.target_position->x.val * as<f32>(content_rect.width()));
                } break;
                default:
                    MLE_UNREACHABLE_LOG("Unexpected target position type: {}", cinfo.target_position->x.type);
            }
            switch (cinfo.target_position->y.type) {
                case TargetBound::Type::DEFAULT:
                case TargetBound::Type::PX: {
                    cinfo.bounds.bounds.pos.y += as<int>(cinfo.target_position->y.val);
                } break;
                case TargetBound::Type::PARENT: {
                    cinfo.bounds.bounds.pos.y += as<int>(cinfo.target_position->y.val * as<f32>(content_rect.height()));
                } break;
                default:
                    MLE_UNREACHABLE_LOG("Unexpected target position type: {}", cinfo.target_position->y.type);
            }
        }

        cinfo.bounds.bounds.pos.x += content_rect.pos.x;
        cinfo.bounds.bounds.pos.y += content_rect.pos.y;

        if (cinfo.origin) {
            cinfo.bounds.bounds.pos.x -= as<int>(cinfo.origin->origin.x * as<f32>(cinfo.bounds.bounds.size.x));
            cinfo.bounds.bounds.pos.y -= as<int>(cinfo.origin->origin.y * as<f32>(cinfo.bounds.bounds.size.y));
        }

        changed_entities.emplace(children[i]);
    }

    for (auto e : changed_entities) {
        auto* e_container = reg.try_get<Container>(e);
        if (e_container) {
            e_container->updateChildrenBounds(e, content_rect, false, true);
        }
    }

    changed_bounds_children_.clear();
}
}  // namespace mle::ui::element::comp
