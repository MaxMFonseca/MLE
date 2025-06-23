#pragma once

#include "../Types.h"
#include "mle/common/Color.h"
#include "mle/renderer/RenderingThread.h"

namespace mle::ui::element {
struct RenderContext {
    renderer::RenderingThreadRef thread{};
    entt::entity self{};
    entt::entity parent{};
};

struct RenderableImpl {
    MLE_NO_COPY_MOVE(RenderableImpl)

    RenderableImpl() = default;
    virtual ~RenderableImpl() = default;

    virtual void apply(entt::entity self, const sol::object& obj) = 0;
    virtual void render(const RenderContext& ctx) const = 0;

    [[nodiscard]] virtual vec2i getSize() const = 0;
    [[nodiscard]] virtual f32 getAspectRatio() const { return as<f32>(getSize().x) / as<f32>(getSize().y); }
};
using RenderableImplHnd = std::unique_ptr<RenderableImpl>;

namespace comp {
struct Renderable {
    MLE_NO_COPY_MOVE(Renderable)

    virtual ~Renderable() = default;
    Renderable() = default;
    explicit Renderable(RenderableImplHnd impl) :
        i(std::move(impl)) {}

    void apply(entt::entity self, const sol::object& obj) const;
    void render(RenderContext ctx) const;

    [[nodiscard]] auto getSize() const { return i ? i->getSize() : vec2i{0}; }
    [[nodiscard]] auto getAspectRatio() const { return i ? i->getAspectRatio() : 0.0F; }

    static Renderable* add(entt::entity e, RenderableImplHnd impl);

    RenderableImplHnd i{};
};

}  // namespace comp
}  // namespace mle::ui::element
