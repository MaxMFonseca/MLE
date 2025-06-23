#pragma once

#include "Renderable.h"

namespace mle::ui::element {
class Sprite : public RenderableImpl {
  public:
    MLE_NO_COPY_MOVE(Sprite)

    Sprite() = default;
    ~Sprite() override = default;

    void render(const RenderContext& ctx) const override;

    void setTexture(entt::entity self, const std::string& texture_name);

    void setColor(const sol::object& o);
    void setColor(Color color);

    void apply(entt::entity self, const sol::object& o) override;

    static renderer::PipelineRef getPipeline();

    [[nodiscard]] vec2i getSize() const override { return texture_.image->getExtent(); };

  private:
    Color color_ = Color::WHITE;
    renderer::Texture texture_;
};
}  // namespace mle::ui::element
