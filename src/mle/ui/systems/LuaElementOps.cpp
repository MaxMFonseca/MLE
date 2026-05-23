#include "LuaElementOps.h"

#include <filesystem>
#include <sol/forward.hpp>

#include "mle/client/Client.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Shader.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Bounds.h"
#include "mle/ui/components/FreeContainer.h"
#include "mle/ui/components/Hoverable.h"
#include "mle/ui/components/ListContainer.h"
#include "mle/ui/renderable/RenderImage.h"
#include "mle/ui/renderable/Sprite.h"
#include "mle/ui/renderable/Text.h"
#include "mle/ui/shader/Blur.h"
#include "mle/ui/shader/LuaShader.h"
#include "mle/utils/String.h"

namespace mle::ui::system {
std::unordered_map<std::string, LuaElementOps::ApplyKeyFn> LuaElementOps::apply_key_handlers_;
std::unordered_map<std::string, LuaElementOps::GetKeyFn> LuaElementOps::get_key_handlers_;

void LuaElementOps::applyTable(const Entt& ew, const sol::table& table) {
    if (const auto children_base_r = table["children_base"]; lua::valid<sol::table>(children_base_r)) {
        ew.emplaceOrReplace<comp::ChildrenBase>(lua::as<sol::table>(children_base_r));
    }

    if (const auto styles_r = table["style"]; lua::valid<sol::table>(styles_r)) {
        for (const auto& [_, value_r] : lua::as<sol::table>(styles_r)) {
            if (value_r.is<std::string>()) {
                auto style_r = ew.ui().getStyle(lua::as<std::string>(value_r));
                if (!style_r) {
                    MLE_E("Style '{}' not found.", lua::as<std::string>(value_r));
                }
                ew.applyTable(style_r->get());
                continue;
            }
            if (value_r.is<sol::table>()) {
                ew.applyTable(lua::as<sol::table>(value_r));
                continue;
            }
            MLE_E("Invalid style entry in styles table for entity {}. Skipping.", ew.fullName());
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
        applyObj(ew, key, value_r);
    }
}

void LuaElementOps::applyObj(const Entt& ew, const std::string& key, const sol::object& obj) {
    if (key == "comp") {
        MLE_ASSERT_LOG(lua::valid<sol::table>(obj), "Invalid 'comp' value for entity {}. Expected table.", ew.fullName());
        auto comp_table = lua::as<sol::table>(obj);
        ew.applyTable(comp_table);
    } else {
        auto it = apply_key_handlers_.find(key);
        if (it != apply_key_handlers_.end()) {
            it->second(ew, obj);
        } else if (!matchAny(key, "name", "idx", "styles", "style")) {
            MLE_E("No handler for element key '{}'", key);
        }
    }
}

sol::object LuaElementOps::getKey(const Entt& ew, const std::string& key, const sol::object& params) {
    auto it = get_key_handlers_.find(key);
    if (it != get_key_handlers_.end()) {
        return it->second(ew, params);
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

void LuaElementOps::init() {
    static bool initialized = false;
    if (initialized) {
        return;
    }
    initialized = true;

    auto& lua = Client::i().lua();

    auto ut = lua.newUsertype<Entt>("mle_ui_Entt", sol::no_constructor);
    ut["apply"] = &Entt::apply;
    ut["get"] = &Entt::getKey;
    ut["entt"] = &Entt::e;
    ut["parent"] = &Entt::parentEw;
    ut["fullName"] = &Entt::fullName;
    ut["getChild"] = [](const Entt& ew, const std::string& name) {
        auto& relationship = ew.getRelationship();
        auto g_r = relationship.getChildByName(ew, name);
        if (g_r == entt::null) {
            MLE_E("Child '{}' not found in entity '{}'", name, ew.fullName());
            return ew.derive(entt::null);
        }
        return ew.derive(g_r);
    };
    ut["addChild"] = [](const Entt& ew, const sol::table& table) {
        auto& relationship = ew.getRelationship();
        relationship.createChild(ew, table);
    };
    ut["applyOnChildren"] = [](const Entt& ew, const sol::table& table) {
        auto& relationship = ew.getRelationship();
        relationship.applyOnChildren(ew, table);
    };
    ut["enableAll"] = [](const Entt& ew) {
        auto& relationship = ew.getRelationship();
        relationship.enableAll(ew);
    };
    ut["disableAll"] = [](const Entt& ew) {
        auto& relationship = ew.getRelationship();
        relationship.disableAll(ew);
    };
    ut["disableAllBut"] = [](const Entt& ew, const std::string& child_name) {
        auto& relationship = ew.getRelationship();
        relationship.disableAllBut(ew, child_name);
    };

    ut["call"] = [](const Entt& ew, const std::string& fn_name, const sol::object& obj = {}) { ew.call(fn_name, obj); };

    ut["dispatch"] = [](const Entt& ew, const std::string& event_name, const sol::object& obj = {}) { ew.dispatch(event_name, obj); };

    ut["destroy"] = &Entt::destroy;

    ut["beginCursorDrag"] = [](const Entt& ew) { ew.addFlag<comp::CursorDragFlag>(); };

    ut["createPopup"] = &Entt::createPopup;
    ut["getBoundsOnRoot"] = &Entt::getBoundsOnRoot;
    ut["getBoundsOnRootNormalized"] = &Entt::getBoundsOnRootNormalized;

    ut["requestInternalBoundsUpdate"] = &Entt::requestInternalBoundsUpdate;
    ut["requestExternalBoundsUpdate"] = &Entt::requestExternalBoundsUpdate;

    comp::Hovered::makeLuaUsertype(lua);
    auto image_ut = lua.newUsertype<Image>("mle_Image", sol::no_constructor);
    image_ut["getExtent"] = &Image::getExtent;

    addBuiltingApply();
    addBuiltingGetters();
}

void LuaElementOps::addBuiltingApply() {
    addApplyKeyHandler("c", comp::Relationship::applyAddChildren);
    addApplyKeyHandler("children", comp::Relationship::applyAddChildren);
    addApplyKeyHandler("add_child", comp::Relationship::applyAddChildren);
    addApplyKeyHandler("child", [](const Entt& ew, const sol::object& obj) {
        comp::FreeContainer::apply(ew, Client::i().lua().createTable());
        comp::Relationship::applyAddChild(ew, obj);
    });
    addApplyKeyHandler("container", comp::ListContainer::apply);
    addApplyKeyHandler("list", comp::ListContainer::apply);
    addApplyKeyHandler("list_container", comp::ListContainer::apply);
    addApplyKeyHandler("free", comp::FreeContainer::apply);
    addApplyKeyHandler("free_container", comp::FreeContainer::apply);
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
    addApplyKeyHandler("on_scroll", comp::Hoverable::applyOnScroll);
    addApplyKeyHandler("table", comp::Table::apply);
    addApplyKeyHandler("render_scale", comp::RenderScale::apply);
    addApplyKeyHandler("on_update", comp::OnUpdate::apply);
    addApplyKeyHandler("on_create", comp::OnCreate::apply);
    addApplyKeyHandler("listen", comp::ListenEvents::apply);
    addApplyKeyHandler("id", comp::ID::apply);
    addApplyKeyHandler("layer", comp::Layer::apply);
    addApplyKeyHandler("fn", comp::Functions::apply);
    addApplyKeyHandler("active", comp::DisabledFlag::applyEnabled);
    addApplyKeyHandler("enabled", comp::DisabledFlag::applyEnabled);
    addApplyKeyHandler("disabled", comp::DisabledFlag::applyDisabled);
    addApplyKeyHandler("force_fit", comp::ForceFitFlag::apply);
    addApplyKeyHandler("add_scroll_y", comp::FreeContainer::applyAddScrollY);
    addApplyKeyHandler("on_resized", comp::OnResized::apply);

    addApplyKeyHandler("sprite", ui::renderable::Sprite::apply);
    addApplyKeyHandler("render_image", ui::renderable::RenderImage::apply);
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
    addGetKeyHandler("fn", comp::Functions::get);
    addGetKeyHandler("overflow", [](const Entt& ew, const sol::object& /*params*/) -> sol::object {
        if (!ew.has<comp::ContentOverflow>()) {
            return {};
        }
        auto table = Client::i().lua().createTable();
        const auto& comp = ew.get<comp::ContentOverflow>();
        table["overflow_x"] = comp.overflow_x;
        table["overflow_y"] = comp.overflow_y;
        return table;
    });
}
}  // namespace mle::ui::system
