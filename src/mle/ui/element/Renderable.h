#pragma once

#include "../Types.h"
#include "mle/common/Color.h"
#include "mle/renderer/RenderingThread.h"

namespace mle::ui::element {
struct RenderContext {
    renderer::RenderingThreadRef thread{};
    Recti current_root_image_bounds{};
    entt::entity self{};
    entt::entity parent{};
};

struct RenderableInterface {
    MLE_NO_COPY_MOVE(RenderableInterface)
    RenderableInterface() = default;
    virtual ~RenderableInterface() = default;

    virtual void renderComp(const RenderContext& ctx) const = 0;
};

using RenderableInterfaceRef = RenderableInterface*;

namespace comp {
struct Renderable {
    using RIGetterFn = RenderableInterface& (*)(entt::entity);
    // We have a getter for the interface instead of storing the object cuz Container is a component
    // We cannot have a pointer to a component in a component (invalidation), so we use a reg getter function.
    // Maybe if I change Container?
    RIGetterFn ri_getter_fn{};

    void render(RenderContext ctx) const;

    static Renderable* add(entt::entity e, RIGetterFn getter_fn);

    vec2i size_{0, 0};
    [[nodiscard]] auto getSize() const { return size_; }
    [[nodiscard]] f32 getAspectRatio() const { return size_.y == 0 ? 0.0F : as<f32>(size_.x) / as<f32>(size_.y); }
};

struct RootImage {
    renderer::ImageHnd image_handle{};
    Color clear_color{};

    static void lkh(entt::entity self, const sol::object& o);
};
}  // namespace comp
}  // namespace mle::ui::element
