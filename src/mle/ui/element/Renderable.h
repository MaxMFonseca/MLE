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
    explicit Renderable(RenderableInterface& ri) :
        ri(ri) {}

    RenderableInterface& ri;

    void render(entt::entity self, renderer::RenderingThreadRef thread) const;

    static void add(entt::entity e, RenderableInterface& ri);
};

struct RootImage {
    renderer::ImageHnd image_handle{};
    Color clear_color{};

    static void lkh(entt::entity self, const sol::object& o);
};
}  // namespace comp
}  // namespace mle::ui::element
