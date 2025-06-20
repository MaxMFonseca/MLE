#pragma once

#include "mle/common/Utils.h"
#include "mle/lua/Types.h"
#include "mle/renderer/Types.h"
#include "mle/ui/Font.h"
#include "mle/ui/Types.h"
#include "mle/ui/element/Renderable.h"

namespace mle::ui::element::comp {
class Text : public RenderableInterface {
  public:
    enum class Justify : u8 { RIGHT, LEFT, CENTER, JUSTIFY };

  public:
    MLE_NO_COPY_MOVE(Text)

    Text() = default;
    ~Text() override = default;

    void renderComp(const RenderContext& ctx) const override;

    void setFont(const std::string& font_name = "");
    void setHeightPx(u32 height_px = 0);
    void setJustify(Justify justify = Justify::JUSTIFY);
    void setWrap(bool wrap = true);
    void setColor(Color color);
    void setText(std::string text);
    void updateText(entt::entity self);

    static void lkh(entt::entity self, const sol::object& o);

    static renderer::PipelineRef getPipeline();

  private:
    std::string text_;
    FontRef font_{};
    Color color_{};
    u32 text_height_px_ = 0;  // 0 means fit
    Justify justify_ = Justify::LEFT;
    bool wrap_ = false;
    Font::RenderText render_text_;
};
}  // namespace mle::ui::element::comp
