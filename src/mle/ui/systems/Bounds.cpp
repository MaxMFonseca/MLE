#include "Bounds.h"

#include "../components/Bounds.h"
#include "mle/core/Logger.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Container.h"

namespace mle::ui::system {
using namespace entt::literals;  // NOLINT

Bounds::Bounds(UI& ui) :
    ui_(ui),
    external_bounds_changed_storage_(ui.getRegistry().storage<entt::reactive>("external_bounds_changed_storage"_hs)),
    needs_internal_update_storage_(ui.getRegistry().storage<entt::reactive>("needs_internal_update_storage"_hs)) {
    MLE_I("Creating Bounds system");

    external_bounds_changed_storage_.on_construct<comp::RequestExternalBoundsUpdateFlag>()
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

    needs_internal_update_storage_.on_construct<comp::ContainerNeedsInternalBoundsUpdateFlag>();
}

void Bounds::update() {
    if (external_bounds_changed_storage_.empty() && needs_internal_update_storage_.empty()) {
        return;
    }

    std::set<entt::entity> containers_to_update;

    for (auto [e] : external_bounds_changed_storage_.each()) {
        Entt ee{ui_, e};
        auto& relationship = ee.getRelationship();
        if (relationship.getParent() != entt::null) {
            containers_to_update.insert(relationship.getParent());
        } else {
            containers_to_update.insert(e);
        }
    }
    external_bounds_changed_storage_.clear();

    for (auto [e] : needs_internal_update_storage_.each()) {
        containers_to_update.insert(e);
    }
    needs_internal_update_storage_.clear();

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

    auto& self_relationship = ee.getRelationship();
    auto self_children = self_relationship.getChildren(ee);

    for (const auto& child : self_children) {
        Entt cee(ui_, child);
        if (cee.getRelationship().getChildCount() > 0) {
            checkContainerNeedsUpdate(child, containers_to_update);
        }
    }
}

void Bounds::updateContainerInternalBounds(entt::entity e) {
    Entt ee(ui_, e);
    std::ignore = ee.get<comp::Container>().calculateChildrenBounds(ee, ee.get<comp::Bounds>().parent_px.size());
};

}  // namespace mle::ui::system
