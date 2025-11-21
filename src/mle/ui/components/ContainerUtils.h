#pragma once

#include "mle/ui/Entt.h"
#include "mle/ui/components/Bounds.h"

namespace mle::ui::comp {
struct ChildBoundsCalcData {
    comp::Bounds& bounds;
    struct {
        comp::TargetPosition position{};
        comp::TargetSize size{};
        comp::TargetMargin margin{};
        comp::TargetBorder border{};
        f32 aspect_ratio = 0.0F;
        vec2f origin = {0.0F, 0.0F};
    } target;

    comp::SizeProvider* size_provider = nullptr;

    vec2i new_size{};
    vec2i new_position{};
    struct {
        int t, b, l, r;
    } new_margin{}, new_border{};

    bool has_target_border = false;

    explicit ChildBoundsCalcData(const Entt& e) :
        bounds(e.get<comp::Bounds>()),
        size_provider(e.tryGet<comp::SizeProvider>()) {
        MLE_T("Creating CBCD for child: {}", e.fullName());
        MLE_T("Has size provider: {}", size_provider != nullptr);
        if (const auto* target_size = e.tryGet<comp::TargetSize>(); target_size) {
            target.size = *target_size;
            MLE_VT(target.size);
        }
        if (const auto* target_position = e.tryGet<comp::TargetPosition>(); target_position) {
            target.position = *target_position;
            MLE_VT(target.position);
        }
        if (const auto* target_margin = e.tryGet<comp::TargetMargin>(); target_margin) {
            target.margin = *target_margin;
            MLE_VT(target.margin);
        }
        if (const auto* target_border = e.tryGet<comp::TargetBorder>(); target_border) {
            target.border = *target_border;
            has_target_border = true;
            MLE_VT(target.border);
        }
        if (const auto* ar_comp = e.tryGet<comp::TargetAspectRatio>(); ar_comp) {
            target.aspect_ratio = ar_comp->o;
            MLE_T("Aspect Ratio: {}", target.aspect_ratio);
        }
        if (const auto* origin_comp = e.tryGet<comp::TargetOrigin>(); origin_comp) {
            target.origin = origin_comp->o;
            MLE_T("Origin: {}", target.origin);
        }
    };
};

using CBCDs = std::map<entt::entity, ChildBoundsCalcData>;

void finishChildBounds(const Entt& centt, ChildBoundsCalcData& cbcd, PaddingPx padding_px);
void finishChildrenBounds(const Entt& e, CBCDs& cbcds, std::span<const entt::entity> to_update, PaddingPx padding_px);
std::pair<vec2i, vec2i> findChildrenMaxMin(const CBCDs& cbcds, bool pack_children);

}  // namespace mle::ui::comp
