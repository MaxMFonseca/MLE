#pragma once

#include "../Element.h"
#include "mle/renderer/Types.h"

namespace mle::ui::element_renderable {
class Sprite final : public Element::Renderable {
  public:
    Sprite(Sprite&&) = delete;
    Sprite& operator=(Sprite&&) = delete;
    Sprite(const Sprite&) = delete;
    Sprite& operator=(const Sprite&) = delete;

    explicit Sprite(ElementRef element);
    ~Sprite() override = default;

    void set(const sol::table& obj) override;
    void getRenderData(int layer, RenderableData& data) const override;
    void updateInternalBounds(std::optional<Element::NewBounds> new_bounds) override;

    [[nodiscard]] f32 getAspectRatio() override { return texture_.aspect_ratio; }
    [[nodiscard]] auto getColor() const { return color_; }

    void setTexture(const std::string& texture_name);
    void setColor(const sol::object& obj);
    void setColor(const Color& color);

    void log(const std::string& prefix) const override;

    static void registerLuaTypes();

  private:
    renderer::Texture texture_{};
    Rectf texture_rect_clamped_ = {0.0F, 0.0F, 1.0F, 1.0F};
    Color color_{1.0F, 1.0F, 1.0F, 1.0F};
    bool visible_ = false;
};
}  // namespace mle::ui::element_renderable
