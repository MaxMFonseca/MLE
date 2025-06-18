#pragma once

#include "../Types.h"
#include "mle/common/Color.h"
#include "mle/renderer/RenderingThread.h"

namespace mle::ui::element {
struct RenderableInterface {
    MLE_NO_COPY_MOVE(RenderableInterface)
    RenderableInterface() = default;
    virtual ~RenderableInterface() = default;

    virtual void renderComp(entt::entity self, renderer::RenderingThreadRef thread) const = 0;
};

using RenderableInterfaceRef = RenderableInterface*;

namespace comp {
struct Renderable {
    using RIGetterFn = RenderableInterface& (*)(entt::entity);
    // We store have a getter for the interface instead of storing the object cuz Container is a component
    // We cannot have a pointer to a component in a component (invalidation), so we use a reg getter function.
    // Maybe if I change Container?
    RIGetterFn ri_getter_fn;

    void render(entt::entity self, renderer::RenderingThreadRef thread) const;

    static void add(entt::entity e, RIGetterFn getter_fn);
};

struct RootImage {
    renderer::ImageHnd image_handle{};
    Color clear_color{};

    static void lkh(entt::entity self, const sol::object& o);
};
}  // namespace comp
}  // namespace mle::ui::element
