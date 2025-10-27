#include "Bounds.h"

#include "../components/Bounds.h"
#include "mle/core/Logger.h"
#include "mle/core/PerfTracker.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Container.h"
#include "mle/utils/ECS.h"

namespace mle::ui::system {
using namespace entt::literals;  // NOLINT

Bounds::Bounds(UI& ui) :
    ui_(ui) {
    MLE_I("Creating Bounds system");
}

void Bounds::update() {
    auto requesting_external_bounds_update_view = ui_.getRegistry().view<comp::RequestExternalBoundsUpdateFlag>();
    if (!requesting_external_bounds_update_view.empty()) {
        for (auto [e] : requesting_external_bounds_update_view.each()) {
            Entt ew{ui_, e};
            ew.requestExternalBoundsUpdate();
        }
        ui_.getRegistry().clear<comp::RequestExternalBoundsUpdateFlag>();
    }

    auto needs_internal_update_view = ui_.getRegistry().view<comp::RequestInternalBoundsUpdateFlag>();
    if (needs_internal_update_view.empty()) {
        return;
    }
    if (needs_internal_update_view->size() == 1) {
        updateEntity({ui_, *needs_internal_update_view->begin()});
    } else {
        updateTree(ui_.getRoot());
    }

    ui_.getRegistry().view<comp::SizeProvider>().each([](entt::entity /*entity*/, comp::SizeProvider& sp) { sp.reset(); });
    ui_.getRegistry().clear<comp::RequestInternalBoundsUpdateFlag>();
}

void Bounds::updateEntity(const Entt& ew) {
    if (!ew.has<comp::RequestInternalBoundsUpdateFlag>()) {
        return;
    }

    auto* size_provider = ew.tryGet<comp::SizeProvider>();
    if (!size_provider) {
        return;
    }

    auto& bounds = ew.get<comp::Bounds>();
    size_provider->call(ew, bounds.parent_px.size());
};

// NOLINTNEXTLINE(misc-no-recursion) Tree traversal
void Bounds::updateTree(entt::entity root) {
    Entt ew{ui_, root};

    updateEntity(ew);

    const auto& relations = ew.getRelationship();
    if (relations.getChildCount() > 0) {
        for (const auto& child : relations.getChildren(ew)) {
            updateTree(child);
        }
    }
};
}  // namespace mle::ui::system
