#include "Text.h"

#include "mle/common/Logger.h"
#include "mle/common/Types.h"
#include "mle/common/UnicodeBuffer.h"
#include "mle/common/Utils.h"
#include "mle/lua/Lua.h"
#include "mle/ui/Font.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/window/Window.h"

namespace mle::ui::element_renderable {
Text::Text(ElementRef element) :
    Renderable(element) {
    MLE_LOG_THIS;
}

void Text::set(const sol::table& obj) {
    if (sol::table r_table = obj["text"]; r_table.valid()) {
        if (auto r_font = r_table["font"]; r_font.valid()) {
            setFont(r_font);
        }
        if (!font_) {
            font_ = ui::getFont();
            setTextDirty();
        }

        if (auto r_text = r_table[1]; r_text.valid()) {
            setText(r_text);
        } else if (auto r_text = r_table["text"]; r_text.valid()) {
            setText(r_text);
        }

        if (sol::object r_color = r_table["color"]; r_color.valid()) {
            setColor(r_color);
        }
        if (sol::object r_outline_color = r_table["outline_color"]; r_outline_color.valid()) {
            setOutlineColor(r_outline_color);
        }
        if (auto r_outline_thickness = r_table["outline_thickness"]; r_outline_thickness.valid()) {
            setOutlineThickness(r_outline_thickness);
        }
        if (auto r_char_height = r_table["char_height"]; r_char_height.valid()) {
            setCharHeight(r_char_height);
        }
    }
    MLE_ASSERT_LOG(font_, "Something weird happened and font is not set");
    calculateTextBounds();
}

void Text::setTextDirty() {
    text_dirty_ = true;
    element_->setRequestingInternalBoundsUpdate();
}

void Text::setText(const std::string& text) {
    text_ = UnicodeBuffer{text};
    setTextDirty();
}

void Text::setFont(const std::string& font_name) {
    font_ = ui::getFont(font_name);
    setTextDirty();
}

void Text::setColor(const sol::object& obj) {
    setColor(Color::fromLua(obj));
}

void Text::setColor(const Color& color) {
    auto c = color.toLinear();
    if (c != color_) {
        color_ = c;
        element_->setRequestingRender();
    }
}

void Text::setOutlineColor(const sol::object& obj) {
    setOutlineColor(Color::fromLua(obj));
}

void Text::setOutlineColor(const Color& color) {
    auto c = color.toLinear();
    if (c != outline_color_) {
        outline_color_ = c;
        element_->setRequestingRender();
    }
}

void Text::setOutlineThickness(f32 thickness) {
    if (outline_thickness_ == thickness) {
        return;
    }
    outline_thickness_ = thickness;
    element_->setRequestingRender();
}

void Text::setCharHeight(f32 height) {
    if (char_height_ == height) {
        return;
    }

    char_height_ = height;
    setTextDirty();
}

// TODO: Text::CharPos Text::getHoveredChar() {
//
// }

void Text::updateInternalBounds(std::optional<Element::NewBounds> new_bounds) {
    if (text_dirty_ || new_bounds) {
        calculateTextBounds();
    }

    f32 char_height = char_height_ == 0.0F ? 1.0F : char_height_;
    MLE_E("char_height: {}", char_height);
    // f32 line_max_width = (1.F / char_height) * element_->getAspectRatio();

    auto parent_on_parent_clamped = clamp(element_->getRectOnParent(), {0, 0, 1, 1});
    auto parent_on_parent_clamped_relative_transformation = calculateRelativeTransformation(element_->getRectOnParent(), parent_on_parent_clamped);
    auto parent_rect_on_root = element_->getRectOnRoot();

    f32 curr_y = 0;
    for (auto& line : lines_) {
        MLE_VW(curr_y);
        for (auto& curr_char : line.chars) {
            char32 c = text_.get(curr_char.text_idx_);

            if (c == ' ') {
                continue;
            }

            Rectf on_parent{};
            on_parent.pos.x = curr_char.pos / text_size_.x;
            on_parent.pos.x += curr_char.cp_data.origin.x / text_size_.x;
            on_parent.pos.x /= char_height;

            on_parent.pos.y = curr_y;
            on_parent.pos.y += (font_->getAscent() + curr_char.cp_data.origin.y) * char_height;

            if (on_parent.pos.x >= 1 || on_parent.pos.y >= 1) {
                continue;
            }

            on_parent.size = curr_char.cp_data.size;
            on_parent.size.x /= text_size_.x;
            on_parent.size.x /= char_height;

            on_parent.size.y *= char_height;

            MLE_W((char)c);
            MLE_W(curr_char.pos);
            MLE_W(text_size_);
            MLE_W(on_parent);

            auto on_parent_clamped = clamp(on_parent, parent_on_parent_clamped_relative_transformation);
            if (feq(on_parent_clamped.size.x, 0.0F) || feq(on_parent_clamped.size.y, 0.0F)) {
                MLE_I("Discarding char: {}, on_parent_clamped: {}", (char)c, on_parent_clamped);
                continue;
            }

            auto& sprite = sprites_.emplace_back();
            sprite.texture = curr_char.cp_data.texture;

            sprite.on_parent = on_parent;

            sprite.texture_rect_clamped.pos = sprite.texture.rect.pos;
            sprite.texture_rect_clamped.pos += ((on_parent_clamped.pos - sprite.on_parent.pos) / sprite.on_parent.size) * sprite.texture.rect.size;
            sprite.texture_rect_clamped.size = sprite.texture.rect.size * (on_parent_clamped.size / sprite.on_parent.size);

            sprite.on_root.pos = parent_rect_on_root.pos + parent_rect_on_root.size * on_parent_clamped.pos;
            sprite.on_root.size = parent_rect_on_root.size * on_parent_clamped.size;
        }

        curr_y += (1 + font_->getLineGap()) * char_height_;
        if (curr_y > 1) {
            break;
        }
    }

    // auto text_height = text_height_ != 0 ? text_height_ : 1 / getRTextMaxH();
    //
    // f32 max_heigh = text_height * getRTextMaxH();
    // f32 max_width = max_heigh * text_aspect_ratio_ / element_->getAspectRatio();
    // r_text_bounds_.size = {max_width, max_heigh};
    // f32 r_width = r_text_bounds_.size.x / r_text_.max_width;
    // r_text_bounds_.pos = -vec2f{text_origin_.x * max_width, text_origin_.y * max_heigh};
    //
    // auto parent_on_parent_clamped = clamp(element_->getRectOnParent(), {0, 0, 1, 1});
    // auto parent_on_parent_clamped_relative_transformation = calculateRelativeTransformation(element_->getRectOnParent(), parent_on_parent_clamped);
    // auto parent_rect_on_root = element_->getRectOnRoot();
    //
    // f32 current_y = 0;
    // for (auto& line : r_text_.lines) {
    //     for (int j = 0; j < static_cast<int>(line.chars.size()); j++) {
    //         char32 c = line.text.get(j);
    //         auto& r_char = line.chars.at(j);
    //
    //         if (c != ' ') {
    //             Rectf on_parent{};
    //             on_parent.pos = r_text_bounds_.pos;
    //             on_parent.pos.x += r_char.on_line.pos.x * r_width;
    //             on_parent.pos.y += current_y + r_char.on_line.pos.y * text_height;
    //             on_parent.size.x = r_char.on_line.size.x * r_width;
    //             on_parent.size.y = r_char.on_line.size.y * text_height;
    //             if (on_parent.pos.x >= 1 || (on_parent.pos.y >= 1)) {
    //                 // MLE_I("Discarding char: {}, on_parent: {}", (char)c, on_parent);
    //                 continue;
    //             }
    //
    //             auto on_parent_clamped = clamp(on_parent, parent_on_parent_clamped_relative_transformation);
    //             if (feq(on_parent_clamped.size.x, 0.0F) || feq(on_parent_clamped.size.y, 0.0F)) {
    //                 // MLE_I("Discarding char: {}, on_parent_clamped: {}", (char)c, on_parent_clamped);
    //                 continue;
    //             }
    //
    //             auto& sprite = sprites_.emplace_back();
    //             sprite.texture = r_char.texture;
    //
    //             sprite.on_parent = on_parent;
    //
    //             sprite.texture_rect_clamped.pos = sprite.texture.rect.pos;
    //             sprite.texture_rect_clamped.pos += ((on_parent_clamped.pos - sprite.on_parent.pos) / sprite.on_parent.size) * sprite.texture.rect.size;
    //             sprite.texture_rect_clamped.size = sprite.texture.rect.size * (on_parent_clamped.size / sprite.on_parent.size);
    //
    //             sprite.on_root.pos = parent_rect_on_root.pos + parent_rect_on_root.size * on_parent_clamped.pos;
    //             sprite.on_root.size = parent_rect_on_root.size * on_parent_clamped.size;
    //         }
    //     }
    //
    //     current_y += (1 + font_->getLineGap()) * text_height;
    //
    //     if (current_y > 1) {
    //         break;
    //     }
    // }
}

void Text::calculateTextBounds() {
    lines_.clear();
    if (text_.empty()) {
        return;
    }
    lines_.emplace_back();

    f32 char_height = char_height_ == 0.0F ? 1.0F : char_height_;
    // f32 line_max_width = (1.F / char_height_) * element_->getAspectRatio();

    text_size_.x = 0;
    for (usize i = 0; i < text_.getSize(); ++i) {
        auto curr_cp = text_.get(i);
        auto& curr_line = lines_.back();

        if (curr_cp == '\n') {
            text_size_.x = std::max(text_size_.x, curr_line.width);
            lines_.emplace_back();
            MLE_E(MLE_1V(1));
            continue;
        }

        auto& curr_char = curr_line.chars.emplace_back();
        curr_char.text_idx_ = i;
        curr_char.cp_data = font_->getCodepointData(curr_cp);

        if (curr_line.chars.size() == 1) {
            curr_char.pos = 0.1F;
        } else {
            curr_char.pos = curr_line.width - 0.1F;
            curr_char.pos += font_->getGlyphKernAdvance(curr_line.chars.at(curr_line.chars.size() - 2).cp_data.glyph_idx, curr_char.cp_data.glyph_idx);
        }

        curr_line.width += curr_char.cp_data.advance;
    }
    text_size_.x = std::max(text_size_.x, lines_.back().width);

    text_size_.y = (static_cast<f32>(lines_.size()) + ((static_cast<f32>(lines_.size()) - 1) * font_->getLineGap())) * char_height;
    MLE_W("text_size_: {}", text_size_);
    MLE_W(getAspectRatio());

    text_dirty_ = false;
}

void Text::getRenderData(int layer, RenderableData& data) const {
    auto& objs = data[layer].texts;
    usize idx = objs.size();
    objs.resize(sprites_.size());

    auto color = color_.withAMultiplied(element_->getOpacity());
    auto outline_color = outline_color_.withAMultiplied(element_->getOpacity());

    for (const auto& sprite : sprites_) {
        auto& obj = objs[idx++];
        obj.pos = sprite.on_root.pos;
        obj.size = sprite.on_root.size;
        obj.color = color;
        obj.texture_offset = sprite.texture_rect_clamped.pos;
        obj.texture_size = sprite.texture_rect_clamped.size;
        obj.texture_idx = sprite.texture.idx;
        obj.outline_thickness = outline_thickness_;
        obj.outline_color = outline_color;
    }
}

void Text::log(const std::string& prefix) const {
    MLE_D("{}  Text. font: {}, color: {}, outline_color: {}, outline_thickness: {}", prefix, font_->getName(), color_, outline_color_, outline_thickness_);
}

void Text::registerLuaTypes() {
    void (Text::*set_color_o)(const sol::object&) = &Text::setColor;
    void (Text::*set_outline_color_o)(const sol::object&) = &Text::setOutlineColor;

    auto text_ut = lua::newUsertype<Text>("ui_Element_Renderable_Text", sol::no_constructor, sol::base_classes, sol::bases<Element::Renderable>());

    text_ut["color"] = sol::property(&Text::getColor, set_color_o);
    text_ut["outline_color"] = sol::property(&Text::getOutlineColor, set_outline_color_o);
    text_ut["outline_thickness"] = sol::property(&Text::getOutlineThickness, &Text::setOutlineThickness);

    text_ut["getText"] = &Text::getText;
    // TODO: text_ut["getHoveredChar"] = &Text::getHoveredChar;
}
}  // namespace mle::ui::element_renderable
