#include "Sprite.h"

#include "../UI.h"
#include "mle/ui/element_renderable/FlexContainer.h"

namespace mle::ui::element_renderable {
Sprite::Sprite(ElementRef element) :
    Element::Renderable(element) {
    MLE_LOG_THIS;
}

void Sprite::set(const sol::table& obj) {
    if (sol::table r_sprite = obj["sprite"]; r_sprite.valid()) {
        if (const auto r_texture = r_sprite[1]; r_texture.valid()) {
            setTexture(r_texture);
        } else if (const auto r_texture = r_sprite["texture"]; r_texture.valid()) {
            setTexture(r_texture);
        } else {
            MLE_UNREACHABLE_LOG("No texture found in table");
        }

        if (sol::object r_color = r_sprite["color"]; r_color.valid()) {
            setColor(r_color);
        }
    }
}

void Sprite::setTexture(const std::string& texture_name) {
    auto new_texture = ui::getTexture(texture_name);
    if (new_texture == texture_) {
        return;
    }
    texture_ = new_texture;
    element_->setRequestingExternalBoundsUpdate();
}

void Sprite::setColor(const sol::object& obj) {
    setColor(Color::fromLua(obj));
}

void Sprite::setColor(const Color& color) {
    auto c = color.toLinear();
    if (c != color_) {
        color_ = c;
        element_->setRequestingRender();
    }
}

void Sprite::updateInternalBounds(std::optional<Element::NewBounds> new_bounds) {
    if (!new_bounds) {
        return;
    }

    Rectf relative_transform = element_->getRectInternalClamped();

    texture_rect_clamped_.pos = texture_.rect.pos + relative_transform.pos * texture_.rect.size;
    texture_rect_clamped_.size = relative_transform.size * texture_.rect.size;

    visible_ = !(texture_rect_clamped_.size.x <= 0.0F || texture_rect_clamped_.size.y <= 0.0);

    element_->setRequestingRender();
}

void Sprite::getRenderData(int layer, RenderableData& data) const {
    SpriteInstance si;
    auto r = element_->getRectOnRootClamped();
    si.pos = r.pos;
    si.size = r.size;
    si.color = color_.withAMultiplied(element_->getOpacity());
    si.texture_idx = texture_.idx;
    si.texture_offset = texture_rect_clamped_.pos;
    si.texture_size = texture_rect_clamped_.size;

    data[layer].sprites.push_back(si);
}

void Sprite::log(const std::string& prefix) const {
    MLE_D("{}  Sprite. texture: {}, color: {}, visible: {}", prefix, texture_, color_, visible_);
}

void Sprite::registerLuaTypes() {
    void (Sprite::*set_color_o)(const sol::object&) = &Sprite::setColor;

    auto ut = lua::newUsertype<Sprite>("ui_Element_Renderable_Sprite", sol::no_constructor, sol::base_classes, sol::bases<Element::Renderable>());

    ut["color"] = sol::property(&Sprite::getColor, set_color_o);
}
}  // namespace mle::ui::element_renderable
