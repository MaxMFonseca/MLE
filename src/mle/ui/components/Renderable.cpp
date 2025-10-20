#include "Renderable.h"

#include "../Entt.h"
#include "mle/core/Logger.h"
#include "mle/ui/components/Bounds.h"

namespace mle::ui::comp {
void Renderable::on_construct(entt::registry& registry, const entt::entity entt) {
    if (registry.any_of<SizeProvider>(entt)) {
        MLE_W("An element shouldnt have multiple SizeProvider components.");
        return;
    }
    registry.emplace<SizeProvider>(entt, [](const Entt& e, vec2u max_size) { return e.get<comp::Renderable>().impl->calculateBounds(max_size); });
}

void Renderable::on_destroy(entt::registry& registry, const entt::entity entt) {
    registry.remove<SizeProvider>(entt);
};

}  // namespace mle::ui::comp
