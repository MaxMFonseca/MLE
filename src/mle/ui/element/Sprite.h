#pragma once

#include "Renderable.h"

namespace mle::ui::element::comp {
class Sprite : public RenderableInterface {
  public:
    void renderComp(entt::entity /*self*/, renderer::RenderingThreadRef /*thread*/) const override{};

    void setTexture(const std::string& texture_name);

    void setColor(const sol::object& o);
    void setColor(Color color);

    static void lkh(entt::entity self, const sol::object& o);

  private:
    Color color_;
    renderer::Texture texture_;
};
}  // namespace mle::ui::element::comp
