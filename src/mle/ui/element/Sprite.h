#pragma once

#include "Renderable.h"

namespace mle::ui::element::comp {
class Sprite : public RenderableInterface {
  public:
    void renderComp(const RenderContext& ctx) const override;

    void setTexture(entt::entity self, const std::string& texture_name);

    void setColor(const sol::object& o);
    void setColor(Color color);

    static void lkh(entt::entity self, const sol::object& o);

    static renderer::PipelineRef getPipeline();

  private:
    Color color_ = Color::WHITE;
    renderer::Texture texture_;
};
}  // namespace mle::ui::element::comp
