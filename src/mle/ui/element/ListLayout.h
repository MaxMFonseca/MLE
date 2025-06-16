#pragma once

#include "Container.h"
#include "mle/ui/element/Base.h"

namespace mle::ui::element {
struct ListLayout : Layout {
    struct ChildBuildInfo {
        ChildBuildInfo(entt::registry& r, entt::entity e, Recti parent_rect);

        comp::Bounds& bounds;
        const comp::TargetSize* target_size;
        const comp::TargetPosition* target_position;
        const comp::TargetMargin* target_margin;
        // const comp::TargetPadding* target_padding;
        // const comp::Origin* origin;

        struct {
            int t{}, b{}, l{}, r{};
        } margin_px;
        struct {
            f32 t{}, b{}, l{}, r{};
        } margin_flex;
        vec2i content_px{0};
        vec2f content_flex{0};
        f32 aspect_ratio = 0.0F;
    };

    static void lkhList(entt::entity self, const sol::object& obj);

    Axis axis = Axis::Y;
    int child_gap = 0;  // FIXME: this should be TargetBound

    void updateChildrenBounds(entt::entity self, Recti content_rect, bool force_update) const override;
    void updateChildrenBoundsY(entt::entity self, Recti content_rect, bool force_update) const;
    void updateChildrenBoundsX(entt::entity self, Recti content_rect, bool force_update) const;
};
}  // namespace mle::ui::element
