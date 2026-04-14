#include "Bounds.h"

#include "../components/Bounds.h"
#include "mle/core/Logger.h"
#include "mle/core/PerfTracker.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Base.h"
#include "mle/utils/ECS.h"

namespace mle::ui::system {
using namespace entt::literals;  // NOLINT

Bounds::Bounds(UI& ui) :
    ui_(ui) {
    MLE_I("Creating Bounds system");
}

void Bounds::update() {
    auto root = ui_.getRoot();
    if (root == entt::null) {
        return;
    }

    if (ui_.getRegistry().all_of<comp::RequestExternalBoundsUpdateFlag>(root)) {
        updateRoot();
    }

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
        updateTree(root);
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

void Bounds::updateRoot() {
    Entt ew{ui_, ui_.getRoot()};

    vec2u root_size_px = ui_.getRootMaxSize();
    if (auto* root_ts = ew.tryGet<comp::TargetSize>()) {
        bool is_fit_width = false;
        bool is_fit_height = false;

        switch (root_ts->x.type) {
            case TargetBound::Type::PX: {
                root_size_px.x = as<u32>(root_ts->x.val);
            } break;
            case TargetBound::Type::DEFAULT: {
                if (root_ts->x.val != 0) {
                    root_size_px.x = as<u32>(as<f32>(ui_.getRootMaxSize().x) * root_ts->x.val);
                } else {
                    is_fit_width = true;
                }
            } break;
            case TargetBound::Type::FIT: {
                is_fit_width = true;
            } break;
            case TargetBound::Type::RELATIVE_W:
            case TargetBound::Type::RELATIVE:
            case TargetBound::Type::ROOT:
            case TargetBound::Type::FLEX_SHARE: {
                root_size_px.x = as<u32>(as<f32>(ui_.getRootMaxSize().x) * (root_ts->x.val >= 0 ? root_ts->x.val : 1.F));
            } break;
            case TargetBound::Type::RELATIVE_H: {
                root_size_px.x = as<u32>(as<f32>(ui_.getRootMaxSize().y) * (root_ts->x.val >= 0 ? root_ts->x.val : 1.F));
            } break;
        }

        switch (root_ts->y.type) {
            case TargetBound::Type::PX: {
                root_size_px.y = as<u32>(root_ts->y.val);
            } break;
            case TargetBound::Type::DEFAULT: {
                if (root_ts->y.val != 0) {
                    root_size_px.y = as<u32>(as<f32>(ui_.getRootMaxSize().y) * root_ts->y.val);
                } else {
                    is_fit_height = true;
                }
            } break;
            case TargetBound::Type::FIT: {
                is_fit_height = true;
            } break;
            case TargetBound::Type::RELATIVE_H:
            case TargetBound::Type::RELATIVE:
            case TargetBound::Type::ROOT:
            case TargetBound::Type::FLEX_SHARE: {
                root_size_px.y = as<u32>(as<f32>(ui_.getRootMaxSize().y) * (root_ts->y.val >= 0 ? root_ts->y.val : 1.F));
            } break;
            case TargetBound::Type::RELATIVE_W: {
                root_size_px.y = as<u32>(as<f32>(ui_.getRootMaxSize().x) * (root_ts->y.val >= 0 ? root_ts->y.val : 1.F));
            } break;
        }

        if (is_fit_width || is_fit_height) {
            MLE_ASSERT(ew.has<comp::SizeProvider>());
            auto& sp = ew.get<comp::SizeProvider>();

            auto fit_size = sp.call(ew, root_size_px);
            if (is_fit_width) {
                root_size_px.x = fit_size.x;
            }
            if (is_fit_height) {
                root_size_px.y = fit_size.y;
            }
        }
    }

    ew.patchOrEmplace<comp::Bounds>([&](comp::Bounds& b) { b.parent_px.setSize(root_size_px); });

    ew.erase<comp::RequestExternalBoundsUpdateFlag>();
    ew.addFlag<comp::RequestInternalBoundsUpdateFlag>();
}
}  // namespace mle::ui::system
