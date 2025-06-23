#include "Container.h"

#include <algorithm>

#include "mle/common/Assert.h"
#include "mle/common/math/Types.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/Base.h"
#include "mle/ui/element/Bounds.h"
#include "mle/ui/element/LuaKeyHandlers.h"
#include "mle/ui/element/Renderable.h"
#include "sol/function_types_templated.hpp"
#include "sol/types.hpp"

namespace mle::ui::element::comp {
struct ChildBuildInfo {
    ChildBuildInfo(entt::registry& r, entt::entity e, Recti parent_rect);

    comp::Bounds& bounds;
    const comp::TargetSize* target_size;
    const comp::TargetPosition* target_position;
    const comp::TargetMargin* target_margin;
    const comp::TargetRelations* target_relations;
    const comp::TargetOrigin* target_origin;
    comp::Container* container;
    comp::Renderable* renderable = nullptr;

    struct {
        int t{}, b{}, l{}, r{};
    } margin_px;
    struct {
        f32 t{}, b{}, l{}, r{};
    } margin_flex;
    vec2i content_px{0};
    vec2f content_flex{0};
    f32 aspect_ratio = 0.0F;

    bool is_flex = false;
};

ChildBuildInfo::ChildBuildInfo(entt::registry& r, entt::entity e, Recti parent_rect) :
    bounds(r.get<Bounds>(e)),
    target_size(r.try_get<TargetSize>(e)),
    target_position(r.try_get<TargetPosition>(e)),
    target_margin(r.try_get<TargetMargin>(e)),
    target_relations(r.try_get<TargetRelations>(e)),
    target_origin(r.try_get<TargetOrigin>(e)),
    container(r.try_get<Container>(e)),
    renderable(r.try_get<Renderable>(e)) {
    //
    using BType = comp::TargetBound::Type;

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

    is_flex = !((target_position != nullptr) || (target_relations != nullptr) || (target_origin != nullptr));
}

void Container::apply(entt::entity self, const sol::object& obj) {
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

    if (const auto align_r = table["align"]; align_r.valid()) {
        MLE_ASSERT(align_r.is<std::string>());
        auto align_str = toLower(align_r.get<std::string>());
        if (align_str == "start") {
            align_cross_ = AlignCross::START;
        } else if (align_str == "end") {
            align_cross_ = AlignCross::END;
        } else if (align_str == "center") {
            align_cross_ = AlignCross::CENTER;
        } else if (align_str == "stretch") {
            align_cross_ = AlignCross::STRETCH;
        } else if (align_str == "baseline") {
            align_cross_ = AlignCross::BASELINE;
        } else {
            MLE_UNREACHABLE_LOG("Unexpected align string: {}", align_str);
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
    auto child = reg.create();

    std::string name;
    if (lua::tryGetIdx(table, 1, name)) {
        reg.emplace_or_replace<comp::Name>(child, std::move(name));
    }

    reg.emplace<comp::Bounds>(child);

    reg.emplace<comp::Parent>(child, self);

    applyEntityTable(child, table);

    children_.add(child, pos);

    notifyChildChangedBounds(self, child);
}

void Container::notifyChildChangedBounds(entt::entity child) {  // NOLINT
    notifyChildChangedBounds(getRegistry().get<Parent>(child).parent, child);
}

void Container::notifyChildChangedBounds(entt::entity self, entt::entity child) {  // NOLINT
    auto& reg = getRegistry();
    auto& self_container = reg.get<Container>(self);
    self_container.children_req_update_.emplace(child);

    auto* target_size = reg.try_get<TargetSize>(self);

    if (target_size && (target_size->x.type == TargetBound::Type::FIT || target_size->y.type == TargetBound::Type::FIT)) {
        notifyChildChangedBounds(self);
    } else {
        reg.emplace_or_replace<ChildChangedBounds>(self);  // Only mark non fit containers with this so full tree traversal may not be needed
    }
}

void Container::render(const RenderContext& ctx) const {
    auto& reg = getRegistry();
    auto children = children_.get();

    auto self_viewport = ctx.thread->getViewport();

    RenderContext child_ctx = ctx;
    child_ctx.parent = ctx.self;

    for (auto child : children) {
        child_ctx.self = child;

        auto& child_bounds = reg.get<Bounds>(child);

        const auto* bg = reg.try_get<Background>(child);
        const auto* renderable = reg.try_get<Renderable>(child);
        auto* blur = reg.try_get<Blur>(child);

        if (bg || renderable || blur) {
            auto viewport = child_bounds.parent_px.asF32();
            viewport.pos += self_viewport.pos;
            ctx.thread->setViewport(viewport);
            // TODO: scissor
        }

        if (blur) {
            blur->render(child_ctx);
        }
        if (bg) {
            bg->render(child_ctx);
        }
        if (renderable) {
            renderable->render(child_ctx);
        }
    }
}

void Container::updateChildrenBounds(entt::entity self, vec2u max_size, bool, bool) {  // NOLINT
    auto& reg = getRegistry();
    auto& container = reg.get<comp::Container>(self);
    auto children = container.getChildren();

    if (!children_req_update_.empty()) {
        Recti padded{0, 0, as<int>(max_size.x), as<int>(max_size.y)};
        std::array<int, 4> padding = {0, 0, 0, 0};
        auto* padding_comp = reg.try_get<Padding>(self);
        if (padding_comp) {
            padding = padding_comp->calcOnRect(padded);
            padded.pos.x += padding[as<usize>(BoxFace::L)];
            padded.pos.y += padding[as<usize>(BoxFace::T)];
            padded.size.x -= padding[as<usize>(BoxFace::L)] + padding[as<usize>(BoxFace::R)];
            padded.size.y -= padding[as<usize>(BoxFace::T)] + padding[as<usize>(BoxFace::B)];
        }

        std::vector<ChildBuildInfo> cinfos;  // TODO: stack allocate this
        cinfos.reserve(children.size());
        std::set<entt::entity> changed_children;

        bool flex_need_update = false;

        for (auto c : children) {
            // I want to only init children that are required
            // But this is not a critical path so I will leave it this way for now
            auto& cinfo = cinfos.emplace_back(reg, c, padded);
            if (cinfo.is_flex && children_req_update_.contains(c)) {
                flex_need_update = true;
            }
        }

        if (flex_need_update) {
            updateChildrenBoundsFlex({
                .self = self,
                .padded = padded,
                .cinfos = cinfos,
                .changed_children = changed_children,
            });
        }

        for (usize i = 0; i < children.size(); i++) {
            auto& cinfo = cinfos.at(i);

            if (cinfo.is_flex) {
                continue;
            }

            bool need_update = children_req_update_.contains(children[i]);
            if (!need_update && cinfo.target_relations) {
                for (auto v : cinfo.target_relations->deps) {
                    need_update |= changed_children.contains(v.e);
                }
            }
            if (!need_update) {
                continue;
            }

            if (cinfo.target_relations) {
                for (const auto& r : cinfo.target_relations->deps) {
                    auto eidx = getChildIdx(r.e);
                    MLE_ASSERT(eidx != max<usize>() && eidx < i);
                    auto& target_bounds = cinfos[eidx].bounds.parent_px;
                    if (r.type == TargetRelations::Dep::Type::SIZE_X) {
                        cinfo.bounds.parent_px.size.x = as<int>(r.val) * target_bounds.size.x;
                    } else if (r.type == TargetRelations::Dep::Type::SIZE_Y) {
                        cinfo.bounds.parent_px.size.y = as<int>(r.val) * target_bounds.size.y;
                    } else if (r.type == TargetRelations::Dep::Type::POS_X) {
                        cinfo.bounds.parent_px.pos.x = target_bounds.pos.x + as<int>(r.val * as<f32>(target_bounds.size.x));
                    } else if (r.type == TargetRelations::Dep::Type::POS_Y) {
                        cinfo.bounds.parent_px.pos.y = target_bounds.pos.y + as<int>(r.val * as<f32>(target_bounds.size.y));
                    }
                }
            }

            if (cinfo.target_size) {
                switch (cinfo.target_size->x.type) {
                    using Type = TargetBound::Type;
                    case Type::DEFAULT:
                    case Type::PX: {
                        cinfo.bounds.parent_px.size.x += as<int>(cinfo.target_size->x.val);
                    } break;
                    case Type::PARENT: {
                        cinfo.bounds.parent_px.size.x += as<int>(cinfo.target_size->x.val * as<f32>(padded.size.x));
                    } break;
                    case Type::FIT: {
                        MLE_ASSERT(cinfo.target_position->x.type == Type::DEFAULT || cinfo.target_position->x.type == Type::PX);
                        cinfo.bounds.parent_px.size.x += padded.size.x - as<int>(cinfo.target_position->x.val);
                    } break;
                    case Type::SELF_H:
                    case Type::SELF:
                        break;
                    default: {
                        MLE_UNREACHABLE_LOG("Unexpected target size type: {}", cinfo.target_size->x.type);
                    }
                }

                switch (cinfo.target_size->y.type) {
                    case TargetBound::Type::DEFAULT:
                    case TargetBound::Type::PX: {
                        cinfo.bounds.parent_px.size.y += as<int>(cinfo.target_size->y.val);
                    } break;
                    case TargetBound::Type::PARENT: {
                        cinfo.bounds.parent_px.size.y += as<int>(cinfo.target_size->y.val * as<f32>(padded.size.y));
                    } break;
                    case TargetBound::Type::FIT: {
                        MLE_ASSERT(cinfo.target_position->y.type == TargetBound::Type::DEFAULT || cinfo.target_position->y.type == TargetBound::Type::PX);
                        cinfo.bounds.parent_px.size.y += padded.size.y - as<int>(cinfo.target_position->y.val);
                    } break;
                    case TargetBound::Type::SELF_W:
                    case TargetBound::Type::SELF:
                        break;
                    default: {
                        MLE_UNREACHABLE_LOG("Unexpected target size type: {}", cinfo.target_size->y.type);
                    }
                }
            }

            if (cinfo.aspect_ratio != 0) {
                if (cinfo.bounds.parent_px.size.x == 0) {
                    cinfo.bounds.parent_px.size.x = as<int>(as<f32>(cinfo.bounds.parent_px.size.y) * cinfo.aspect_ratio);
                }
                if (cinfo.bounds.parent_px.size.y == 0) {
                    cinfo.bounds.parent_px.size.y = as<int>(as<f32>(cinfo.bounds.parent_px.size.x) / cinfo.aspect_ratio);
                }
            }

            if (cinfo.target_position) {
                switch (cinfo.target_position->x.type) {
                    case TargetBound::Type::DEFAULT:
                    case TargetBound::Type::PX: {
                        cinfo.bounds.parent_px.pos.x += as<int>(cinfo.target_position->x.val);
                    } break;
                    case TargetBound::Type::PARENT: {
                        cinfo.bounds.parent_px.pos.x += as<int>(cinfo.target_position->x.val * as<f32>(padded.size.x));
                    } break;
                    default:
                        MLE_UNREACHABLE_LOG("Unexpected target position type: {}", cinfo.target_position->x.type);
                }
                switch (cinfo.target_position->y.type) {
                    case TargetBound::Type::DEFAULT:
                    case TargetBound::Type::PX: {
                        cinfo.bounds.parent_px.pos.y += as<int>(cinfo.target_position->y.val);
                    } break;
                    case TargetBound::Type::PARENT: {
                        cinfo.bounds.parent_px.pos.y += as<int>(cinfo.target_position->y.val * as<f32>(padded.size.y));
                    } break;
                    default:
                        MLE_UNREACHABLE_LOG("Unexpected target position type: {}", cinfo.target_position->y.type);
                }
            }

            if (cinfo.container) {
                bool fit_x = false;
                bool fit_y = false;

                auto max_size = padded.size;
                if (cinfo.bounds.parent_px.size.x > 0) {
                    max_size.x = cinfo.bounds.parent_px.size.x;
                } else {
                    max_size.x = padded.size.x - cinfo.bounds.parent_px.pos.x;
                    fit_x = true;
                }
                if (cinfo.bounds.parent_px.size.y > 0) {
                    max_size.y = cinfo.bounds.parent_px.size.y;
                } else {
                    max_size.y = padded.size.y - cinfo.bounds.parent_px.pos.y;
                    fit_y = true;
                }
                cinfo.container->updateChildrenBounds(children[i], max_size);

                if (fit_x || fit_y) {
                    auto span = cinfo.container->getChildrenSpan();

                    if (fit_x) {
                        cinfo.bounds.parent_px.size.x = span.size.x;
                    }
                    if (fit_y) {
                        cinfo.bounds.parent_px.size.y = span.size.y;
                    }
                }
            }

            if (cinfo.bounds.parent_px.size.x == 0) {
                cinfo.bounds.parent_px.size.x = cinfo.renderable->getSize().x;
            }
            if (cinfo.bounds.parent_px.size.y == 0) {
                cinfo.bounds.parent_px.size.y = cinfo.renderable->getSize().y;
            }

            MLE_ASSERT(cinfo.bounds.parent_px.size.x > 0 && cinfo.bounds.parent_px.size.y > 0);

            // if (c_container) {
            //     c_container->alignChildren(cinfo.bounds.parent_px.size);
            // }

            vec2i origin{0};

            if (cinfo.target_origin) {
                origin.x = as<int>(cinfo.target_origin->v.x * as<f32>(cinfo.bounds.parent_px.size.x));
                origin.y = as<int>(cinfo.target_origin->v.y * as<f32>(cinfo.bounds.parent_px.size.y));
            }

            // if (c_container) {
            //     auto move = (cinfo.bounds.parent_px.pos - padded.pos) * vec2i{fit_x, fit_y};
            //     move -= origin;
            //     c_container->moveChildren(move);
            // }

            cinfo.bounds.parent_px.pos.x -= origin.x;
            cinfo.bounds.parent_px.pos.y -= origin.y;

            changed_children.emplace(children[i]);
        }

        calculateChildrenSpan(cinfos, padding);
    }

    children_req_update_.clear();
}

void Container::calculateChildrenSpan(const std::vector<ChildBuildInfo>& cinfos, std::array<int, 4> padding) {
    int min_x = max<int>();
    int min_y = max<int>();
    int max_x = min<int>();
    int max_y = min<int>();

    for (auto child : cinfos) {
        auto& cbounds = child.bounds;
        min_x = std::min(min_x, cbounds.parent_px.left());
        min_y = std::min(min_y, cbounds.parent_px.top());
        max_x = std::max(max_x, cbounds.parent_px.right());
        max_y = std::max(max_y, cbounds.parent_px.bottom());
    }

    if (min_x > max_x || min_y > max_y) {
        children_span_ = Recti{0, 0, 0, 0};
    }

    Recti ret{min_x, min_y, max_x - min_x, max_y - min_y};

    ret.pos.x -= padding[as<usize>(BoxFace::L)];
    ret.pos.y -= padding[as<usize>(BoxFace::T)];
    ret.size.x += padding[as<usize>(BoxFace::L)] + padding[as<usize>(BoxFace::R)];
    ret.size.y += padding[as<usize>(BoxFace::T)] + padding[as<usize>(BoxFace::B)];

    children_span_ = ret;
}

namespace {
void updateChildrenBoundsColCross(ChildBuildInfo& cinfo, Recti content_rect) {
    f32 off_flex_shares = cinfo.content_flex.x + cinfo.margin_flex.r + cinfo.margin_flex.l;
    f32 off_remaining = as<f32>(content_rect.width() - cinfo.margin_px.r - cinfo.margin_px.l - cinfo.content_px.x);
    f32 off_flex_value_px = off_flex_shares != 0.0F && off_remaining != 0.0F ? off_remaining / std::max(1.0F, off_flex_shares) : 0.0F;

    cinfo.bounds.parent_px.pos.x = content_rect.left() + cinfo.margin_px.l;
    cinfo.bounds.parent_px.pos.x += as<int>(cinfo.margin_flex.l * off_flex_value_px);

    cinfo.bounds.parent_px.size.x = cinfo.content_px.x + as<int>(cinfo.content_flex.x * off_flex_value_px);
}
}  // namespace

void Container::updateChildrenBoundsFlex(FlexUpdateData data) {
    auto children = children_.get();

    int main_axis_size = 0;
    f32 main_axis_flex_shares = 0.0F;
    int non_immutable_children_cound = 0;

    for (usize i = 0; i < children.size(); i++) {
        auto& cinfo = data.cinfos[i];
        if (!cinfo.is_flex) {
            continue;
        }

        bool main_is_default = cinfo.content_flex.y == 0.0F && cinfo.content_px.y == 0;
        bool off_is_default = cinfo.content_flex.x == 0.0F && cinfo.content_px.x == 0;

        if (!off_is_default) {
            updateChildrenBoundsColCross(cinfo, data.padded);
        }

        if (main_is_default) {
            if (off_is_default || cinfo.aspect_ratio == 0.0F) {
                cinfo.content_flex.y = 1.0F;
                main_is_default = false;
            } else {
                cinfo.content_px.y = as<int>(as<f32>(cinfo.bounds.parent_px.size.x) / cinfo.aspect_ratio);
            }
        }

        main_axis_size += cinfo.content_px.y + cinfo.margin_px.t + cinfo.margin_px.b;
        main_axis_flex_shares += cinfo.content_flex.y + cinfo.margin_flex.t + cinfo.margin_flex.b;
        non_immutable_children_cound++;
    }

    main_axis_size += (non_immutable_children_cound - 1) * gap_;

    f32 main_axis_remaining_px = as<f32>(data.padded.size.y - main_axis_size);
    f32 main_axis_share_value_px = 0.0F;
    if (main_axis_remaining_px < 0) {
        MLE_TODO;
    } else if (main_axis_remaining_px != 0 && main_axis_flex_shares != 0.0F) {
        main_axis_share_value_px = as<f32>(main_axis_remaining_px) / std::max(1.0F, main_axis_flex_shares);
    }

    int main_axis_pos = data.padded.pos.y;
    for (usize i = 0; i < children.size(); i++) {
        auto& cinfo = data.cinfos[i];

        if (!cinfo.is_flex) {
            continue;
        }

        cinfo.margin_px.t += as<int>(cinfo.margin_flex.t * main_axis_share_value_px);
        cinfo.margin_px.b += as<int>(cinfo.margin_flex.b * main_axis_share_value_px);
        cinfo.content_px.y += as<int>(cinfo.content_flex.y * main_axis_share_value_px);

        cinfo.bounds.parent_px.pos.y = main_axis_pos + cinfo.margin_px.t;
        cinfo.bounds.parent_px.size.y = cinfo.content_px.y;

        main_axis_pos = cinfo.bounds.parent_px.pos.y + cinfo.bounds.parent_px.size.y + cinfo.margin_px.b + gap_;

        if (cinfo.bounds.parent_px.size.x == 0) {
            if (cinfo.aspect_ratio != 0) {
                cinfo.content_px.x = as<int>(as<f32>(cinfo.bounds.parent_px.size.y) * cinfo.aspect_ratio);
            } else {
                cinfo.content_flex.x = 1.0F;
            }

            updateChildrenBoundsColCross(cinfo, data.padded);
        }

        MLE_ASSERT(cinfo.bounds.parent_px.size.x > 0 && cinfo.bounds.parent_px.size.y > 0);

        data.changed_children.emplace(children[i]);
    }
}
}  // namespace mle::ui::element::comp
