#pragma once

#include "Renderable.h"
#include "mle/renderer/Pipeline.h"

namespace mle::ui::element {
// This is suposed to be a Lua shader only.
// If the app wants a custom renderable it should use a custom RenderableImpl
class Shader : public RenderableImpl {
  public:
    MLE_NO_COPY_MOVE(Shader)

    Shader() = default;
    ~Shader() override = default;

    void render(const RenderContext& ctx) const override;

    void apply(entt::entity self, const sol::object& o) override;

    [[nodiscard]] vec2i getSize() const override { return {100, 100}; }

  private:
    renderer::PipelineHnd pipeline_;
    sol::function fn_;
};
}  // namespace mle::ui::element
