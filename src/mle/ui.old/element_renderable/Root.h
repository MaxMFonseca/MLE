#pragma once

#include "../Element.h"
#include "FlexContainer.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Types.h"

/////////////// FIXME: THIS WILL BECOME DefaultRoot and Root will be promoted to Element Core ///////////////
namespace mle::ui::element_renderable {
class Root : public FlexContainer {
  public:
    Root(Root&&) = delete;
    Root& operator=(Root&&) = delete;
    Root(const Root&) = delete;
    Root& operator=(const Root&) = delete;

    explicit Root(ElementRef element);
    ~Root() override = default;

    void set(const sol::table& table) override;
    void updateInternalBounds(std::optional<Element::NewBounds> new_bounds) override;
    bool render() override;
    void getRenderData(int layer, RenderableData& data) const override;
    void renderExtra();
    virtual void postProc() {};

    virtual void createImages();

    void log(const std::string& prefix) const override;

    renderer::ImageRef getImage() { return image_.get(); };

    void setClearColor(const Color& color) {
        clear_color_ = color;
        need_render_internal_ = true;
    }
    void setClearColor(const sol::object& obj) { setClearColor(Color::fromLua(obj)); }

    void setFormat(vk::Format format);
    [[nodiscard]] auto getFormat() const { return format_; }

    static Root& get(ElementRef e);

  private:
    void createDefaultImage();

  protected:
    renderer::ImageHnd image_;
    vk::Format format_{};
    Color clear_color_ = Color::ZERO;
    Color tint_ = Color::WHITE;

    Rectf texture_rect_clamped_ = {0.0F, 0.0F, 1.0F, 1.0F};
    bool visible_ = false;
    bool images_dirty_ = true;
    bool need_render_internal_ = true;
};
}  // namespace mle::ui::element_renderable
