#include "LuaElementOps.h"

#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Bounds.h"
#include "mle/utils/String.h"

namespace mle::ui::system {
entt::entity LuaElementOps::createElement(entt::entity parent) {
    auto e = ui_.getRegistry().create();
    if (parent != entt::null) {
        ui_.getRegistry().emplace<comp::Parent>(e, parent);
    }
    ui_.getRegistry().emplace<comp::Bounds>(e);
    return e;
}

void LuaElementOps::applyTable(entt::entity e, const sol::table& table) {
    for (const auto& [key_r, value_r] : table) {
        if (!key_r.is<std::string>()) {
            MLE_E("Non-string key in element table! Skipping.");
            continue;
        }
        applyObj(e, key_r.as<std::string>(), value_r);
    }
}

void LuaElementOps::applyObj(entt::entity e, const std::string& key, const sol::object& obj) {
    auto it = key_handlers_.find(key);
    if (it != key_handlers_.end()) {
        it->second(Entt(ui_, e), obj);
    } else if (!matchAny(key, "name", "pos")) {
        MLE_E("No handler for element key '{}'", key);
    }
}

void LuaElementOps::addKeyHandler(const std::string& key, KeyHandlerFn fn) {
    MLE_ASSERT_LOG(!key_handlers_.contains(key), "Key handler for '{}' already exists!", key);
    key_handlers_[key] = fn;
};

LuaElementOps::LuaElementOps(UI& ui) :
    ui_(ui) {
    auto& lua = ui.getLua();

    auto ut = lua.newUsertype<Entt>("mle_ui_Entt", sol::no_constructor);
    ut["apply"] = &Entt::apply;

    addKeyHandler("container", comp::Container::apply);
    addKeyHandler("child", comp::Container::applyAddChild);
    addKeyHandler("c", comp::Container::applyAddChildren);
    addKeyHandler("children", comp::Container::applyAddChildren);
    addKeyHandler("size", comp::TargetSize::apply);
    addKeyHandler("size_x", comp::TargetSize::applyX);
    addKeyHandler("size_y", comp::TargetSize::applyY);
    addKeyHandler("size_x_dep", comp::TargetSize::applyXDep);
    addKeyHandler("size_y_dep", comp::TargetSize::applyYDep);
    addKeyHandler("pos", comp::TargetPosition::apply);
    addKeyHandler("pos_x", comp::TargetPosition::applyX);
    addKeyHandler("pos_y", comp::TargetPosition::applyY);
    addKeyHandler("pos_x_dep", comp::TargetPosition::applyXDep);
    addKeyHandler("pos_y_dep", comp::TargetPosition::applyYDep);
    addKeyHandler("padding", comp::TargetPadding::apply);
    addKeyHandler("padding_t", comp::TargetPadding::applyT);
    addKeyHandler("padding_b", comp::TargetPadding::applyB);
    addKeyHandler("padding_l", comp::TargetPadding::applyL);
    addKeyHandler("padding_r", comp::TargetPadding::applyR);
    addKeyHandler("padding_x", comp::TargetPadding::applyX);
    addKeyHandler("padding_y", comp::TargetPadding::applyY);
    addKeyHandler("margin", comp::TargetMargin::apply);
    addKeyHandler("margin_t", comp::TargetMargin::applyT);
    addKeyHandler("margin_b", comp::TargetMargin::applyB);
    addKeyHandler("margin_l", comp::TargetMargin::applyL);
    addKeyHandler("margin_r", comp::TargetMargin::applyR);
    addKeyHandler("margin_x", comp::TargetMargin::applyX);
    addKeyHandler("margin_y", comp::TargetMargin::applyY);
    addKeyHandler("border", comp::TargetBorder::apply);
    addKeyHandler("border_thickness", comp::TargetBorder::applyThickness);
    addKeyHandler("border_color", comp::TargetBorder::applyColor);
    addKeyHandler("border_round", comp::TargetBorder::applyRound);
    addKeyHandler("border_t", comp::TargetBorder::applyT);
    addKeyHandler("border_b", comp::TargetBorder::applyB);
    addKeyHandler("border_l", comp::TargetBorder::applyL);
    addKeyHandler("border_r", comp::TargetBorder::applyR);
    addKeyHandler("border_x", comp::TargetBorder::applyX);
    addKeyHandler("border_y", comp::TargetBorder::applyY);
    addKeyHandler("border_round_lt", comp::TargetBorder::applyRoundLT);
    addKeyHandler("border_round_rt", comp::TargetBorder::applyRoundRT);
    addKeyHandler("border_round_lb", comp::TargetBorder::applyRoundLB);
    addKeyHandler("border_round_rb", comp::TargetBorder::applyRoundRB);
    addKeyHandler("origin", comp::TargetOrigin::apply);
    addKeyHandler("aspect_ratio", comp::TargetAspectRatio::apply);
}
// addLuaKeyHandler("background", background);
// addLuaKeyHandler("blur", blur);
// addLuaKeyHandler("rel", rel);
// addLuaKeyHandler("root_image", rootImage);
// addLuaKeyHandler("sprite", sprite);
// addLuaKeyHandler("text", text);
// addLuaKeyHandler("shader", shader);
// addLuaKeyHandler("table", table);
// addLuaKeyHandler("on_init", onInit);
// addLuaKeyHandler("on_update", onUpdate);
// addLuaKeyHandler("on_hover", collidable<comp::OnHover>);
// addLuaKeyHandler("on_hover_enter", collidable<comp::OnHoverEnter>);
// addLuaKeyHandler("on_hover_leave", collidable<comp::OnHoverLeave>);
// addLuaKeyHandler("on_click", collidable<comp::OnLeftPressed, true>);
// addLuaKeyHandler("on_left_pressed", collidable<comp::OnLeftPressed, true>);
// addLuaKeyHandler("on_left_released", collidable<comp::OnLeftReleased, true>);
// addLuaKeyHandler("on_left_down", collidable<comp::OnLeftDown, true>);
}  // namespace mle::ui::system
