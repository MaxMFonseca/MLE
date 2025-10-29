#include "LuaElementOps.h"

#include <filesystem>

#include "mle/lua/Utils.h"
#include "mle/renderer/Shader.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Bounds.h"
#include "mle/ui/components/Hoverable.h"
#include "mle/ui/renderable/Sprite.h"
#include "mle/ui/renderable/Text.h"
#include "mle/ui/shader/Blur.h"
#include "mle/ui/shader/LuaShader.h"
#include "mle/utils/String.h"

namespace mle::ui::system {
void LuaElementOps::applyTable(entt::entity e, const sol::table& table) {
    if (const auto children_base_r = table["children_base"]; lua::valid<sol::table>(children_base_r)) {
        Entt entt{ui_, e};
        entt.emplaceOrReplace<comp::ChildrenBase>(lua::as<sol::table>(children_base_r));
    }

    if (const auto styles_r = table["style"]; lua::valid<sol::table>(styles_r)) {
        Entt entt{ui_, e};
        for (const auto& [_, value_r] : lua::as<sol::table>(styles_r)) {
            if (value_r.is<std::string>()) {
                auto style_r = ui_.getStyle(lua::as<std::string>(value_r));
                if (!style_r) {
                    MLE_E("Style '{}' not found.", lua::as<std::string>(value_r));
                }
                entt.applyTable(style_r->get());
                continue;
            }
            if (value_r.is<sol::table>()) {
                entt.applyTable(lua::as<sol::table>(value_r));
                continue;
            }
            MLE_E("Invalid style entry in styles table for entity {}. Skipping.", e);
        }
    }

    for (const auto& [key_r, value_r] : table) {
        if (!key_r.is<std::string>()) {
            MLE_E("Non-string key in element table! Skipping.");
            continue;
        }
        auto key = lua::as<std::string>(key_r);
        if (mle::matchAny(key, "children_base")) {
            continue;
        }
        applyObj(e, key, value_r);
    }
}

void LuaElementOps::applyObj(entt::entity e, const std::string& key, const sol::object& obj) {
    auto it = apply_key_handlers_.find(key);
    if (it != apply_key_handlers_.end()) {
        it->second(Entt(ui_, e), obj);
    } else if (!matchAny(key, "name", "idx", "styles", "style")) {
        MLE_E("No handler for element key '{}'", key);
    }
}

sol::object LuaElementOps::getKey(entt::entity e, const std::string& key) {
    auto it = get_key_handlers_.find(key);
    if (it != get_key_handlers_.end()) {
        return it->second(Entt(ui_, e));
    }
    MLE_E("No handler for element key '{}'", key);
    return {};
};

void LuaElementOps::addApplyKeyHandler(const std::string& key, ApplyKeyFn fn) {
    MLE_ASSERT_LOG(!apply_key_handlers_.contains(key), "Key handler for '{}' already exists!", key);
    apply_key_handlers_[key] = fn;
};

void LuaElementOps::addGetKeyHandler(const std::string& key, GetKeyFn fn) {
    MLE_ASSERT_LOG(!get_key_handlers_.contains(key), "Get key handler for '{}' already exists!", key);
    get_key_handlers_[key] = fn;
};

LuaElementOps::LuaElementOps(UI& ui) :
    ui_(ui) {
    auto& lua = ui.getLua();

    auto ut = lua.newUsertype<Entt>("mle_ui_Entt", sol::no_constructor);
    ut["apply"] = &Entt::apply;
    ut["get"] = &Entt::getKey;
    ut["entt"] = &Entt::e;
    ut["fullName"] = &Entt::fullName;

    comp::Hovered::makeLuaUsertype(lua);

    addBuiltingApply();
    addBuiltingGetters();
}

void LuaElementOps::addBuiltingApply() {
    addApplyKeyHandler("c", comp::Relationship::applyAddChildren);
    addApplyKeyHandler("children", comp::Relationship::applyAddChildren);
    addApplyKeyHandler("add_child", comp::Relationship::applyAddChildren);
    addApplyKeyHandler("child", comp::Relationship::applyAddChildren);
    addApplyKeyHandler("container", comp::Container::apply);
    addApplyKeyHandler("size", comp::TargetSize::apply);
    addApplyKeyHandler("size_x", comp::TargetSize::applyX);
    addApplyKeyHandler("size_y", comp::TargetSize::applyY);
    addApplyKeyHandler("size_x_dep", comp::TargetSize::applyXDep);
    addApplyKeyHandler("size_y_dep", comp::TargetSize::applyYDep);
    addApplyKeyHandler("pos", comp::TargetPosition::apply);
    addApplyKeyHandler("pos_x", comp::TargetPosition::applyX);
    addApplyKeyHandler("pos_y", comp::TargetPosition::applyY);
    addApplyKeyHandler("pos_x_dep", comp::TargetPosition::applyXDep);
    addApplyKeyHandler("pos_y_dep", comp::TargetPosition::applyYDep);
    addApplyKeyHandler("padding", comp::TargetPadding::apply);
    addApplyKeyHandler("padding_t", comp::TargetPadding::applyT);
    addApplyKeyHandler("padding_b", comp::TargetPadding::applyB);
    addApplyKeyHandler("padding_l", comp::TargetPadding::applyL);
    addApplyKeyHandler("padding_r", comp::TargetPadding::applyR);
    addApplyKeyHandler("padding_x", comp::TargetPadding::applyX);
    addApplyKeyHandler("padding_y", comp::TargetPadding::applyY);
    addApplyKeyHandler("margin", comp::TargetMargin::apply);
    addApplyKeyHandler("margin_t", comp::TargetMargin::applyT);
    addApplyKeyHandler("margin_b", comp::TargetMargin::applyB);
    addApplyKeyHandler("margin_l", comp::TargetMargin::applyL);
    addApplyKeyHandler("margin_r", comp::TargetMargin::applyR);
    addApplyKeyHandler("margin_x", comp::TargetMargin::applyX);
    addApplyKeyHandler("margin_y", comp::TargetMargin::applyY);
    addApplyKeyHandler("border", comp::TargetBorder::apply);
    addApplyKeyHandler("border_thickness", comp::TargetBorder::applyThickness);
    addApplyKeyHandler("border_color", comp::TargetBorder::applyColor);
    addApplyKeyHandler("border_round", comp::TargetBorder::applyRound);
    addApplyKeyHandler("border_t", comp::TargetBorder::applyT);
    addApplyKeyHandler("border_b", comp::TargetBorder::applyB);
    addApplyKeyHandler("border_l", comp::TargetBorder::applyL);
    addApplyKeyHandler("border_r", comp::TargetBorder::applyR);
    addApplyKeyHandler("border_x", comp::TargetBorder::applyX);
    addApplyKeyHandler("border_y", comp::TargetBorder::applyY);
    addApplyKeyHandler("border_round_lt", comp::TargetBorder::applyRoundLT);
    addApplyKeyHandler("border_round_rt", comp::TargetBorder::applyRoundRT);
    addApplyKeyHandler("border_round_lb", comp::TargetBorder::applyRoundLB);
    addApplyKeyHandler("border_round_rb", comp::TargetBorder::applyRoundRB);
    addApplyKeyHandler("origin", comp::TargetOrigin::apply);
    addApplyKeyHandler("aspect_ratio", comp::TargetAspectRatio::apply);
    addApplyKeyHandler("background", comp::Background::apply);
    addApplyKeyHandler("on_hover", comp::Hoverable::applyOnHover);
    addApplyKeyHandler("on_hover_in", comp::Hoverable::applyOnHoverIn);
    addApplyKeyHandler("on_hover_out", comp::Hoverable::applyOnHoverOut);
    addApplyKeyHandler("on_key", comp::Hoverable::applyOnKey);
    addApplyKeyHandler("on_keys", comp::Hoverable::applyOnKeys);
    addApplyKeyHandler("table", comp::Table::apply);

    addApplyKeyHandler("sprite", ui::renderable::Sprite::apply);
    addApplyKeyHandler("text", ui::renderable::Text::apply);
    addApplyKeyHandler("text_input_enable", ui::renderable::Text::applyInputEnable);
    addApplyKeyHandler("text_input_disable", ui::renderable::Text::applyInputDisable);
    addApplyKeyHandler("text_input_clear", ui::renderable::Text::applyInputClear);

    addApplyKeyHandler("blur", ui::shader::Blur::apply);
    addApplyKeyHandler("shader", ui::shader::LuaShader::apply);
    addApplyKeyHandler("shader_params", ui::shader::LuaShader::applyParams);
}

void LuaElementOps::addBuiltingGetters() {
    addGetKeyHandler("table", comp::Table::get);
    addGetKeyHandler("hovered", comp::Hovered::get);
}
}  // namespace mle::ui::system
