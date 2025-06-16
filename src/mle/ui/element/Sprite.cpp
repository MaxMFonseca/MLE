#include "Sprite.h"

#include "mle/lua/Types.h"
#include "mle/lua/Utils.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"

namespace mle::ui::element::comp {

void Sprite::setTexture(const std::string& texture_name) {
}

void Sprite::setColor(Color color) {
    color_ = color;
}

void Sprite::lkh(entt::entity self, const sol::object& o) {
    MLE_ASSERT(o.valid());
    auto& reg = getRegistry();

    // auto* renderable_comp = reg.try_get<comp::Renderable>(self);
    auto* comp = reg.try_get<Sprite>(self);
    if (!comp) {
        comp = &reg.emplace<Sprite>(self);
    }

    if (o.is<std::string>()) {
        comp->setTexture(o.as<std::string>());
    } else if (o.is<sol::table>()) {
        auto table = o.as<sol::table>();
        std::string texture_name;
        auto lua_get_result = lua::tryGetKeyOrIdx(table, "texture", 1, texture_name);
        MLE_ASSERT(lua_get_result);
        comp->setTexture(texture_name);

        if (const auto color_r = table["color"]; color_r.valid()) {
            comp->setColor(color_r);
        }
    } else {
        MLE_UNREACHABLE_LOG("Unexpected object type for Sprite: {}", o.get_type());
    }
}
}  // namespace mle::ui::element::comp
