#include "Element.h"

#include "mle/lua/Utils.h"
#include "mle/renderer/Image.h"
#include "mle/ui/Types.h"

namespace mle::ui::detail::e_components {
void Name::onLuaKeyName(entt::registry& reg, entt::entity e, const sol::object& obj) {
    auto& component = reg.get_or_emplace<Name>(e);
    component.name = lua::as<std::string>(obj);
}

RootImage::RootImage(const sol::table& table) {
    auto extent = lua::getKey<vec2i>(table, "extent");
    renderer::Image::CI image_ci;
    image_ci.extent = extent;
    image_ci.format = renderer::getDefaultColorFormat();
    image_ci.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
    image_ = renderer::Image::createHnd(image_ci);

    auto color_o = table["clear_color"];
    if (color_o.valid()) {
        clear_color_ = Color::fromLua(color_o);
    }
}
}  // namespace mle::ui::detail::e_components
