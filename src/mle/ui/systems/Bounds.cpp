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

    storage_.on_construct<comp::RequestExternalBoundsUpdate>()
        .on_construct<comp::TargetSize>()
        .on_destroy<comp::TargetSize>()
        .on_update<comp::TargetSize>()
        .on_construct<comp::TargetPosition>()
        .on_destroy<comp::TargetPosition>()
        .on_update<comp::TargetPosition>()
        .on_construct<comp::TargetPadding>()
        .on_destroy<comp::TargetPadding>()
        .on_update<comp::TargetPadding>()
        .on_construct<comp::TargetMargin>()
        .on_destroy<comp::TargetMargin>()
        .on_update<comp::TargetMargin>()
        .on_construct<comp::TargetOrigin>()
        .on_destroy<comp::TargetOrigin>()
        .on_update<comp::TargetOrigin>()
        .on_construct<comp::TargetAspectRatio>()
        .on_destroy<comp::TargetAspectRatio>()
        .on_update<comp::TargetAspectRatio>();
}

void Bounds::update() {
    MLE_T(".");
    if (storage_.empty()) {
        return;
    }
    MLE_D("Updating Bounds system");

    std::set<entt::entity> containers_to_update;
    for (auto [e] : storage_.each()) {
        auto* parent_c = ui_.getRegistry().try_get<comp::Parent>(e);
        if (parent_c && parent_c->o != entt::null) {
            containers_to_update.insert(parent_c->o);
        } else {
            containers_to_update.insert(e);
        }
    }
    auto container_needs_internal_update_view = ui_.getRegistry().view<comp::ContainerNeedsInternalBoundsUpdate>();
    for (auto e : container_needs_internal_update_view) {
        containers_to_update.insert(e);
    }
    ui_.getRegistry().clear<comp::RequestExternalBoundsUpdate>();
    ui_.getRegistry().clear<comp::ContainerNeedsInternalBoundsUpdate>();

    checkContainerNeedsUpdate(ui_.getRoot(), fixFitContainersNeedsUpdate(containers_to_update));
}

std::set<entt::entity> Bounds::fixFitContainersNeedsUpdate(const std::set<entt::entity>& containers_to_update) {
    std::set<entt::entity> ret;
    for (auto e : containers_to_update) {
        Entt ee(ui_, e);
        while (ee.anyFitTargetExternalBound()) {
            ee.setE(ee.getParent());
        }
        ret.insert(ee.e());
    }
    return ret;
}

// NOLINTNEXTLINE(misc-no-recursion) yeah, i know, its cool, its a tree, i cant really avoid it
void Bounds::checkContainerNeedsUpdate(entt::entity e, const std::set<entt::entity>& containers_to_update) {
    if (containers_to_update.contains(e)) {
        updateContainerBounds(e);
        return;
    }

    auto& container = ui_.getRegistry().get<comp::Container>(e);

    for (const auto& child : container.o.get()) {
        if (ui_.getRegistry().try_get<comp::Container>(child.e)) {
            checkContainerNeedsUpdate(child.e, containers_to_update);
        }
    }
}

std::vector<entt::entity> Bounds::sortChildrenByDependency(std::span<const entt::entity> span) {
    enum class Mark : u8 { UNSEEN = 0, VISITING = 1, DONE = 2 };
    std::map<entt::entity, Mark> mark;

    std::vector<entt::entity> out;
    out.reserve(span.size());

    auto find_it = [](auto span, entt::entity e) { return std::find_if(span.begin(), span.end(), [e](entt::entity o) { return o == e; }); };
    // NOLINTNEXTLINE(misc-no-recursion) yeah, cool recursion
    auto dfs = [&](auto&& self, entt::entity e) {
        const auto mark_it = mark.find(e);
        if (mark_it != mark.end()) {
            if (mark_it->second == Mark::DONE) {
                return true;
            }
            if (mark_it->second == Mark::VISITING) {
                // NOLINTNEXTLINE(bugprone-lambda-function-name) just a log
                MLE_ASSERT_LOG(false, "Cycle detected while sorting UI deps at entity {}", e);
                return false;
            }
        }

        mark[e] = Mark::VISITING;

        Entt ee(ui_, e);

        std::array<entt::entity, 4> deps{};
        usize n = 0;
        auto push_unique = [&](entt::entity d) {
            if (d == entt::null) {
                return;
            }
            for (usize i = 0; i < n; ++i) {
                if (deps.at(i) == d) {
                    return;
                }
            }
            if (n < deps.size()) {
                deps.at(n++) = d;
            }
        };

        if (const auto* pos = ee.tryGet<comp::TargetPosition>()) {
            push_unique(pos->xdep.e);
            push_unique(pos->ydep.e);
        }
        if (const auto* size = ee.tryGet<comp::TargetSize>()) {
            push_unique(size->xdep.e);
            push_unique(size->ydep.e);
        }

        for (usize i = 0; i < n; ++i) {
            const entt::entity d = deps.at(i);
            if (find_it(span, d) == span.end()) {
                continue;  // skip deps outside of the span, maybe is a list child
            }
            if (!self(self, d)) {
                return false;
            }
        }

        mark[e] = Mark::DONE;
        out.push_back(e);
        return true;
    };

    for (const auto& e : span) {
        if (!mark.contains(e)) {
            if (!dfs(dfs, e)) {
                MLE_E("Cycle detected while sorting UI deps. Returning empty list.");
                return {};
            }
        }
    }

    MLE_ASSERT_LOG(out.size() == span.size(), "Sorted UI deps size mismatch. Check this. {}x{}", out.size(), span.size());

    return out;
}

std::pair<std::vector<entt::entity>, std::vector<entt::entity>> Bounds::separateFlexFromListChildren(std::span<const EntityStorage::Entry> span) {
    std::vector<entt::entity> flex_children;
    std::vector<entt::entity> list_children;

    for (const auto& e : span) {
        Entt ee{ui_, e.e};
        if (ee.has<comp::TargetPosition>()) {
            flex_children.push_back(e.e);
        } else {
            list_children.push_back(e.e);
        }
    }

    return {flex_children, list_children};
}

void Bounds::updateContainerBounds(entt::entity e) {
    Entt eentt{ui_, e};
    vec2u bottom_size_px = eentt.get<comp::Bounds>().parent_px.size();
    auto& container = eentt.get<comp::Container>();
    auto children = container.o.get();
    auto [flex_children, list_children] = separateFlexFromListChildren(children);
    auto sorted_by_dependencies = sortChildrenByDependency(flex_children);

    for (auto c : list_children) {
        Entt centt{ui_, c};
    }
};
}  // namespace mle::ui::system
