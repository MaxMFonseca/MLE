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
bool contains(const std::vector<entt::entity>& vec, entt::entity e) {
    return std::ranges::find_first_of(vec, std::views::single(e)) != vec.end();
}
}  // namespace

void Hover::update() {
    auto& r = ui_.getRegistry();
    const auto mouse_pos_r = UserInputManager::i().getCursorPos();
    std::vector<entt::entity> inside_stack;
    if (mouse_pos_r) {
        inside_stack.reserve(16);
        auto mouse_px = vec2i(mouse_pos_r.value());
        inside_stack.push_back(ui_.getRoot());
        for (entt::entity e = ui_.getRoot(); e != entt::null;) {
            e = getFirstHoveredChild(e, mouse_px);
            if (e != entt::null) {
                inside_stack.push_back(e);
            }
        }
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

    for (auto e : std::ranges::reverse_view(inside_stack)) {
        auto* hov = r.try_get<comp::Hoverable>(e);
        if (!hov) {
            continue;
        }

        Entt ew{ui_, e};
        auto* h = ew.tryGet<comp::Hovered>();
        if (!h) {
            h = &ew.emplace<comp::Hovered>();
            h->state = HoverState::IN;
            hov->onHoverIn(ew);
            h->state = HoverState::HOVER;
        } else if (h->state != HoverState::HOVER) {
            h->state = HoverState::HOVER;
        }

        hov->onHover(ew);
    }
};

entt::entity Hover::getFirstHoveredChild(entt::entity e, vec2i pos) {
    Entt ew{ui_, e};
    auto& relationship = ew.getRelationship();
    auto& bounds = ew.get<comp::Bounds>();
    pos += bounds.parent_px.pos();
    if (relationship.getChildCount()) {
        auto children = relationship.getChildren(ew);
        for (auto c : children) {
            Entt cew = ew.derive(c);
            if (!(cew.getRelationship().getChildCount() || cew.has<comp::Hoverable>())) {
                continue;
            }
            auto c_bounds = cew.get<comp::Bounds>();
            if (c_bounds.parent_px.contains(pos)) {
                return c;
            }
        }
    }
    return entt::null;
};
}  // namespace mle::ui::system
