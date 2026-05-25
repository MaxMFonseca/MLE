#include "ShaderI.h"

#include "mle/lua/Utils.h"

namespace mle::ui {
void ShaderI::setBase(const sol::table& table) {
    if (const sol::object before_children_r = table["before_children"]; lua::valid<bool>(before_children_r)) {
        before_children = before_children_r.as<bool>();
    }
    if (const sol::object dedicate_render_target_r = table["dedicate_render_target"]; lua::valid<bool>(dedicate_render_target_r)) {
        dedicate_render_target = dedicate_render_target_r.as<bool>();
    }
    if (const sol::object clear_color_r = table["clear_color"]; lua::valid<sol::table>(clear_color_r)) {
        clear_color = Color::fromLua(clear_color_r).toLinear();
    }
}
}  // namespace mle::ui
