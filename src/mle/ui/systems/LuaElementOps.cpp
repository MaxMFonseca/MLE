#include "LuaElementOps.h"

#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Bounds.h"

namespace mle::ui::system {
entt::entity LuaElementOps::createElement(const sol::table& table, entt::entity parent) {
    auto e = ui_.getRegistry().create();
    if (parent != entt::null) {
        ui_.getRegistry().emplace<comp::Parent>(e, parent);
    }
    applyTable(e, table);
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
    } else {
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

    addKeyHandler("size", comp::TargetSize::apply);
    addKeyHandler("size_x", comp::TargetSize::applyX);
    addKeyHandler("size_y", comp::TargetSize::applyY);
    addKeyHandler("pos", comp::TargetPosition::apply);
    addKeyHandler("pos_x", comp::TargetPosition::applyX);
    addKeyHandler("pos_y", comp::TargetPosition::applyY);
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
    addKeyHandler("origin", comp::TargetOrigin::apply);
    addKeyHandler("aspect_ratio", comp::TargetAspectRatio::apply);
    addKeyHandler("relations", comp::TargetRelations::apply);
    addKeyHandler("relation", comp::TargetRelations::applyAdd);
    addKeyHandler("container", comp::Container::apply);
    addKeyHandler("c", comp::Container::apply);
    addKeyHandler("container_add", comp::Container::applyAdd);
    addKeyHandler("c_add", comp::Container::applyAdd);
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
