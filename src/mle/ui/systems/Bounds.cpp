#include "Bounds.h"

#include "../components/Bounds.h"
#include "mle/core/Logger.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Container.h"

namespace mle::ui::system {
Bounds::Bounds(UI& ui) :
    ui_(ui),
    storage_(ui.getRegistry().storage<entt::reactive>()) {
    MLE_I("Creating Bounds system");

    storage_.on_construct<comp::RequestExternalBoundsUpdateFlag>()
        .on_construct<comp::TargetSize>()
        .on_destroy<comp::TargetSize>()
        .on_update<comp::TargetSize>()
        .on_construct<comp::TargetPosition>()
        .on_destroy<comp::TargetPosition>()
        .on_update<comp::TargetPosition>()
        .on_construct<comp::TargetMargin>()
        .on_destroy<comp::TargetMargin>()
        .on_update<comp::TargetMargin>()
        .on_construct<comp::TargetBorder>()
        .on_destroy<comp::TargetBorder>()
        .on_update<comp::TargetBorder>()
        .on_construct<comp::TargetPadding>()
        .on_destroy<comp::TargetPadding>()
        .on_update<comp::TargetPadding>()
        .on_construct<comp::TargetOrigin>()
        .on_destroy<comp::TargetOrigin>()
        .on_update<comp::TargetOrigin>()
        .on_construct<comp::TargetAspectRatio>()
        .on_destroy<comp::TargetAspectRatio>()
        .on_update<comp::TargetAspectRatio>();
}

void Bounds::update() {
    if (storage_.empty()) {
        return;
    }

    std::set<entt::entity> containers_to_update;
    for (auto [e] : storage_.each()) {
        Entt ee{ui_, e};
        auto& relationship = ee.getRelationship();
        if (relationship.parent != entt::null) {
            containers_to_update.insert(relationship.parent);
        } else {
            containers_to_update.insert(e);
        }
    }
    storage_.clear();
    auto container_needs_internal_update_view = ui_.getRegistry().view<comp::ContainerNeedsInternalBoundsUpdateFlag>();
    for (auto e : container_needs_internal_update_view) {
        containers_to_update.insert(e);
    }
    ui_.getRegistry().clear<comp::RequestExternalBoundsUpdateFlag>();
    ui_.getRegistry().clear<comp::ContainerNeedsInternalBoundsUpdateFlag>();

    checkContainerNeedsUpdate(ui_.getRoot(), fixFitContainersNeedsUpdate(containers_to_update));
}

std::set<entt::entity> Bounds::fixFitContainersNeedsUpdate(const std::set<entt::entity>& containers_to_update) {
    std::set<entt::entity> ret;
    for (auto e : containers_to_update) {
        Entt ee(ui_, e);
        while (ee.hasFitSize()) {
            ee.setE(ee.getParent());
        }
        ret.insert(ee.e());
    }
    return ret;
}

// NOLINTNEXTLINE(misc-no-recursion) yeah, i know, its cool, its a tree, i cant really avoid it
void Bounds::checkContainerNeedsUpdate(entt::entity e, const std::set<entt::entity>& containers_to_update) {
    if (containers_to_update.contains(e)) {
        updateContainerInternalBounds(e);
        return;
    }

    Entt ee(ui_, e);

    for (const auto& child : comp::Container::getChildren(ee)) {
        Entt cee(ui_, child);
        if (cee.getRelationship().child_count > 0) {
            checkContainerNeedsUpdate(child, containers_to_update);
        }
    }
}

void Bounds::updateContainerInternalBounds(entt::entity e) {
    Entt ee(ui_, e);
    std::ignore = ee.get<comp::Container>().calculateChildrenBounds(ee, ee.get<comp::Bounds>().parent_px.size());
};

}  // namespace mle::ui::system
