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
    comp::Renderable::add(self, container);
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

void Container::notifyChildChangedBounds(entt::entity self, entt::entity child) {  // NOLINT
    auto& reg = getRegistry();
    auto& self_container = reg.get<Container>(self);
    self_container.changed_bounds_children_.emplace(child);

    if (isFit(self)) {
        notifyChildChangedBounds(reg.get<comp::Parent>(self).parent, self);
    } else {
        reg.emplace<ChildChangedBounds>(self);  // Only mark non fit containers with this so tree traversal may not be needed
    }
}

void Container::renderComp(entt::entity self, renderer::RenderingThreadRef thread) const {
    auto& reg = getRegistry();
    auto children = children_.get();

    // I need to pass in the context of the current root image!@#
    // for rotation

    auto fbounds = reg.get<comp::Bounds>(self).bounds.asF32();

    for (auto child : children) {
        auto fcbounds = reg.get<comp::Bounds>(child).bounds.asF32();

        // render child bg
        if (const auto* bg = reg.try_get<Background>(child); bg) {
            thread->setPipeline(Background::getPipeline());
            struct {
                vec2f pos{};
                vec2f size{};
                vec4f color{};
            } pc;
            // FIXME: the context is wrong here, this should be renderd on the current root
            // the thread does have the context for size, but not pos/rotation
            // also we need to find a way to do clipping
            pc.pos = fcbounds.pos / fbounds.size;
            pc.size = fcbounds.size / fbounds.size;
            MLE_VC(pc.size);
            MLE_VC(pc.pos);
            pc.color = bg->color;
            thread->pushConstants(&pc);
            thread->setViewport();
            thread->draw(1, 4);
        }

        // TODO: render child Renderable
    }
}
}  // namespace mle::ui::element::comp
