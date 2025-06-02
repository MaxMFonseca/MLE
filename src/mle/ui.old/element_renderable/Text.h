#pragma once

#include "../Element.h"
#include "mle/common/Stopwatch.h"
#include "mle/common/UnicodeBuffer.h"
#include "mle/renderer/Types.h"
#include "mle/ui/Font.h"
#include "sol/forward.hpp"

namespace mle::ui::element_renderable {
class Text : public Element::Renderable {
  public:
    Text(Text&&) = delete;
    Text& operator=(Text&&) = delete;
    Text(const Text&) = delete;
    Text& operator=(const Text&) = delete;

    explicit Text(ElementRef element);
    ~Text() override = default;

    void set(const sol::table& obj) override;
    void getRenderData(int layer, RenderableData& data) const override;
    void updateInternalBounds(std::optional<Element::NewBounds> new_bounds) override;
    [[nodiscard]] f32 getAspectRatio() override { return text_size_.x / text_size_.y; }

    auto getText() { return text_.toUTF8(); }
    [[nodiscard]] auto getColor() const { return color_; }
    [[nodiscard]] auto getOutlineColor() const { return outline_color_; }
    [[nodiscard]] auto getOutlineThickness() const { return outline_thickness_; }

    void setTextDirty();
    void setText(const std::string& text);
    void setFont(const std::string& font_name);
    void setColor(const sol::object& obj);
    void setColor(const Color& color);
    void setOutlineColor(const sol::object& obj);
    void setOutlineColor(const Color& color);
    void setOutlineThickness(f32 thickness);
    void setCharHeight(f32 height);

    void calculateTextBounds();

    void log(const std::string& prefix) const override;

    static void registerLuaTypes();

  protected:
    UnicodeBuffer text_{};
    FontRef font_{};

    bool text_dirty_ = false;

    bool wrap_ = false;
    f32 char_height_ = 0;
    vec2f text_size_{0, 0};

    Color color_{1.0F, 1.0F, 1.0F, 1.0F};
    Color outline_color_{0.0F, 0.0F, 0.0F, 1.0F};
    f32 outline_thickness_ = 0.0F;

    struct Line {
        struct Char {
            f32 pos = 0.0;
            usize text_idx_ = 0;
            Font::CodepointData cp_data;
        };

        std::vector<Char> chars;
        f32 width = 0.2;
    };

    std::vector<Line> lines_;

    // TODO: std::shared_ptr<SpecialWordsTransformer> special_words_transformer_;
    // TODO: std::map<usize, CharCustomization> char_customizations_;

    struct Sprite {
        renderer::Texture texture{};
        Rectf texture_rect_clamped = {0.0F, 0.0F, 1.0F, 1.0F};
        Rectf on_parent = {0, 0, 1, 1};
        Rectf on_root = {0, 0, 1, 1};
    };
    std::vector<Sprite> sprites_;
};
}  // namespace mle::ui::element_renderable
