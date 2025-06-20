#include "Container.h"

#include "mle/lua/Utils.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/Renderable.h"

namespace mle::ui::element::comp {
void Container::add(entt::entity self, LayoutHnd&& layout, const sol::table& table) {
    auto& reg = getRegistry();
    auto& container = reg.emplace<comp::Container>(self, std::move(layout));
    container.addChildren(self, table);
    comp::Renderable::add(self, [](entt::entity self) -> RenderableInterface& { return getRegistry().get<Container>(self); });
}

Container::Container(LayoutHnd&& layout) :
    layout_(std::move(layout)) {
}

void Container::addChildren(entt::entity self, const sol::table& table) {
    for (const auto& [key, val] : table) {
        if (key.is<std::string>()) {
            continue;
        }

        addChild(self, val, lua::getKeyOr(val, "pos", max<usize>()));
    }
}

void Container::addChild(entt::entity self, const sol::table& table, usize pos) {
    auto& reg = getRegistry();
    auto child_entt = reg.create();
    applyTable(child_entt, table, self);

    children_.add(child_entt, pos);
}

void Container::updateChildrenBounds(entt::entity self, Recti context, bool verify_all, bool force_update) const {  // NOLINT
    MLE_ASSERT(layout_);
    auto& bounds = getRegistry().get<comp::Bounds>(self).bounds;

    if (!isFitX(self)) {
        context.pos.x = bounds.top();
        context.size.x = bounds.width();
    }
    if (!isFitY(self)) {
        context.pos.y = bounds.left();
        context.size.y = bounds.height();
    }

    if (!changed_bounds_children_.empty()) {
        layout_->updateChildrenBounds(self, context, force_update);
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
        reg.emplace_or_replace<ChildChangedBounds>(self);  // Only mark non fit containers with this so tree traversal may not be needed
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
}  // namespace mle::ui::element::comp
