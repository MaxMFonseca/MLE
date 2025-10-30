#include "Hover.h"

#include <algorithm>
#include <ranges>

#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Bounds.h"
#include "mle/ui/components/Hoverable.h"
#include "mle/utils/ECS.h"
#include "mle/window/UserInputManager.h"

namespace mle::ui::system {
namespace {
bool contains(std::vector<entt::entity>& arr, entt::entity e) {
    return std::ranges::find(arr, e) != arr.end();
}
}  // namespace

void Hover::update() {
    auto& r = ui_.getRegistry();
    const auto mouse_pos_r = UserInputManager::i().getCursorPos();
    std::vector<entt::entity> inside_stack;
    inside_stack.reserve(16);

    if (mouse_pos_r) {
        auto mouse_px = mouse_pos_r.value();
        Entt ew{ui_, ui_.getRoot()};
        hovered(ew, mouse_px, ew.get<comp::Bounds>(), inside_stack);
    }

    std::vector<entt::entity> to_remove;

    r.view<comp::Hovered>().each([&](entt::entity e, comp::Hovered& h) {
        if (!contains(inside_stack, e)) {
            Entt ew{ui_, e};
            // NOLINTNEXTLINE(bugprone-lambda-function-name) to remove
            MLE_ASSERT(ew.has<comp::Hoverable>());
            auto& hov = ew.get<comp::Hoverable>();
            h.state = HoverState::OUT;
            hov.onHoverOut(ew);
            to_remove.push_back(e);
        }
    });

    for (entt::entity e : to_remove) {
        r.remove<comp::Hovered>(e);
    }
    to_remove.clear();
};

// NOLINTNEXTLINE(misc-no-recursion) cool recursive function
void Hover::hovered(const Entt& ew, vec2f pos_parent, const comp::Bounds& self_bounds, std::vector<entt::entity>& inside) {
    vec2f pos_self = pos_parent - vec2f(self_bounds.parent_px.pos());
    inside.push_back(ew.e());

    auto* hov = ew.tryGet<comp::Hoverable>();
    if (hov) {
        vec2f pos_self_norm = pos_self / vec2f(self_bounds.parent_px.size());

        auto* h = ew.tryGet<comp::Hovered>();
        if (!h) {
            h = &ew.emplace<comp::Hovered>();
            h->state = HoverState::IN;
            hov->onHoverIn(ew);
            h->state = HoverState::HOVER;
        } else if (h->state != HoverState::HOVER) {
            h->state = HoverState::HOVER;
        }

        h->pos_self = pos_self;
        h->pos_self_norm = pos_self_norm;
        hov->onHover(ew);
    }

    auto& relationship = ew.getRelationship();
    if (relationship.hasChildren()) {
        auto children = relationship.getChildren(ew);
        for (auto c : children) {
            Entt cew = ew.derive(c);
            if (!(cew.getRelationship().getChildCount() || cew.has<comp::Hoverable>())) {
                continue;
            }
            auto c_bounds = cew.get<comp::Bounds>();
            if (c_bounds.parent_px.contains(pos_self)) {
                hovered(cew, pos_self, c_bounds, inside);
            }
        }
    }
};
}  // namespace mle::ui::system
