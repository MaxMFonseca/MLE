#pragma once

#include <memory>

#include "mle/ui/Types.h"
#include "mle/ui/renderable/RenderableI.h"
#include "mle/utils/ECS.h"

namespace mle::ui::comp {
struct Renderable {
    std::unique_ptr<RenderableI> impl;

    [[nodiscard]] std::unique_ptr<RenderablePacketI> createPacket() const { return impl->createPacket(); }
    void updatePacket(RenderablePacketI* packet) const { impl->updatePacket(packet); }
    [[nodiscard]] vec2u calculateBounds(const Entt& e, vec2u max_size) const { return impl->calculateBounds(e, max_size); }
    [[nodiscard]] entt::id_type getType() const { return impl->getType(); }

    explicit Renderable(std::unique_ptr<RenderableI> impl_) :
        impl(std::move(impl_)) {}

    // NOLINTBEGIN(readability-identifier-naming, readability-avoid-const-params-in-decls) Not my declaration
    static void on_construct(entt::registry& registry, const entt::entity entt);
    static void on_destroy(entt::registry& registry, const entt::entity entt);
    // NOLINTEND(readability-identifier-naming, readability-avoid-const-params-in-decls)
};
}  // namespace mle::ui::comp
