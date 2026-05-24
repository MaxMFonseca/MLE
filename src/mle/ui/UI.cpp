#include "UI.h"

#include "Entt.h"
#include "mle/client/Client.h"
#include "mle/core/PerfTracker.h"
#include "mle/lua/Utils.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Bounds.h"
#include "mle/ui/components/ListContainer.h"
#include "mle/ui/systems/LuaElementOps.h"
#include "mle/window/UserInputManager.h"
#include "mle/window/Window.h"

namespace mle {
UI::UI() {
    window_resize_el_ = Window::i().getED().makeListener<window::ev::Resize>([this](const window::ev::Resize& ev) {
        root_max_size_ = ev.size;
        if (root_ != entt::null) {
            ui::Entt ew{*this, root_};
            ew.requestExternalBoundsUpdate();
        }
    });
    ui::system::LuaElementOps::init();
    root_max_size_ = Window::i().getSize();

    popup_mouse_listener_ = std::make_unique<KeyListener>(
        [this]() {
            handlePopupMousePress();
        },
        Keybinding{.key = Key::MOUSE_ONE, .state = KeyState::PRESSED});
    popup_mouse_listener_->setAlwaysCall(true);
    popup_mouse_listener_->listen();
}

UI::~UI() = default;

void UI::clear() {
    MLE_I("Clearing UI");
    registry_.clear();
    root_ = entt::null;
    root_max_size_ = vec2u{0};
    rendering_system_.clear();
}

void UI::setRoot(sol::table root_table) {
    MLE_I("Setting UI root from table: {}", root_table);

    addRootStyles(root_table["styles"]);

    registry_.clear();

    root_ = registry_.create();
    ui::Entt root_ew{*this, root_};
    root_ew.emplace<ui::comp::Relationship>();
    root_ew.emplace<ui::comp::Bounds>();
    root_ew.applyTable(root_table);
    if (!root_ew.has<ui::comp::SizeProvider>()) {
        root_ew.emplace<ui::comp::ListContainer>();
    }
    root_ew.requestExternalBoundsUpdate();
};

void UI::setRoot(const std::string& element_name) {
    auto root_table = Client::i().lua().require(element_name);
    setRoot(root_table);
};

void UI::addStyle(const std::string& style_name, const sol::table& style_table) {
    MLE_T("Adding style '{}': {}", style_name, style_table);
    styles_.emplace(style_name, style_table);
}

void UI::addRootStyles(const sol::object& obj) {
    if (!lua::valid<sol::table>(obj)) {
        return;
    }

    auto table = lua::as<sol::table>(obj);
    for (const auto& [key_r, value_r] : table) {
        if (!key_r.is<std::string>() || !lua::valid<sol::table>(value_r)) {
            MLE_E("Invalid style entry in root styles table, skipping.");
            continue;
        }
        addStyle(lua::as<std::string>(key_r), lua::as<sol::table>(value_r));
    }
}

void UI::update() {
    MLE_PERF_SCOPE("GUIUpdate");

    {
        MLE_PERF_SCOPE("GUIUpdate.Hover");
        hover_system_.update();
    }

    {
        MLE_PERF_SCOPE("GUIUpdate.Update");
        auto view = registry_.view<ui::comp::OnUpdate>();
        for (auto e : view) {
            ui::Entt ew{*this, e};
            const auto& on_update = view.get<ui::comp::OnUpdate>(e);
            if (on_update.fn) {
                on_update.fn(ew);
            }
        }
    }

    {
        MLE_PERF_SCOPE("GUIUpdate.Events");
        events_.dispatchEvents();
    }

    {
        MLE_PERF_SCOPE("GUIUpdate.Bounds");
        bounds_system_.update();
    }

    {
        MLE_PERF_SCOPE("GUIUpdate.Rendering");
        rendering_system_.update();
    }

    {
        MLE_PERF_SCOPE("GUIUpdate.Destroy");
        destroyFlagged();
    }
}

ImageRef UI::render() {
    MLE_PERF_SCOPE("GUIRender");
    return rendering_system_.render();
};

Expected<ImageRef> UI::getRenderImage(entt::entity entity) {
    return rendering_system_.getRenderImage(entity);
}

[[nodiscard]] Expected<std::reference_wrapper<const sol::table>> UI::getStyle(std::string_view style_name) {
    if (auto it = styles_.find(std::string{style_name}); it != styles_.end()) {
        return std::cref(it->second);
    }
    return std::unexpected(Result::NOT_FOUND);
}

Expected<ui::Entt> UI::getE(std::span<const std::string_view> tree) {
    ui::Entt root_ew{*this, root_};
    if (tree.empty()) {
        return root_ew;
    }
    return root_ew.getChild(tree);
};

Expected<ui::Entt> UI::getE(const std::string& id) {
    for (auto e : registry_.view<ui::comp::ID>()) {
        ui::Entt ew{*this, e};
        const auto& comp_id = ew.get<ui::comp::ID>();
        if (comp_id.o == id) {
            return ew;
        }
    }
    return std::unexpected(Result::NOT_FOUND);
};

entt::entity UI::findNearestPopupRoot(entt::entity entity) {
    entt::entity cur = entity;
    while (cur != entt::null) {
        if (!registry_.valid(cur)) {
            return entt::null;
        }

        ui::Entt ew{*this, cur};
        if (ew.has<ui::comp::PopupRoot>()) {
            return cur;
        }

        cur = ew.get<ui::comp::Relationship>().getParent();
    }

    return entt::null;
}

bool UI::hasPopups() const {
    return !registry_.view<ui::comp::PopupRoot>().empty();
}

u32 UI::getNextPopupStackIndex(entt::entity parent_popup) const {
    if (parent_popup == entt::null || !registry_.valid(parent_popup)) {
        return 1;
    }

    const auto* parent_root = registry_.try_get<ui::comp::PopupRoot>(parent_popup);
    if (!parent_root) {
        return 1;
    }

    return parent_root->stack_index + 1;
}

bool UI::isPopupAncestorOf(entt::entity maybe_ancestor, entt::entity popup) const {
    entt::entity cur = popup;
    while (cur != entt::null) {
        if (cur == maybe_ancestor) {
            return true;
        }

        const auto* popup_root = registry_.try_get<ui::comp::PopupRoot>(cur);
        if (!popup_root) {
            return false;
        }

        cur = popup_root->parent_popup;
    }

    return false;
}

void UI::trimPopupStackTo(entt::entity popup_root) {
    std::vector<entt::entity> to_destroy;

    for (auto popup : registry_.view<ui::comp::PopupRoot>()) {
        if (popup_root == entt::null || !isPopupAncestorOf(popup, popup_root)) {
            to_destroy.push_back(popup);
        }
    }

    for (auto popup : to_destroy) {
        if (registry_.valid(popup)) {
            ui::Entt{*this, popup}.destroy();
        }
    }
}

void UI::handlePopupMousePress() {
    if (!hasPopups()) {
        return;
    }

    const auto mouse_pos_r = UserInputManager::i().getCursorPos();
    if (!mouse_pos_r) {
        trimPopupStackTo(entt::null);
        return;
    }

    entt::entity hit = hover_system_.hitTest(mouse_pos_r.value());
    entt::entity popup_root = hit == entt::null ? entt::null : findNearestPopupRoot(hit);
    trimPopupStackTo(popup_root);
}

void UI::destroyFlagged() {
    std::vector<entt::entity> flagged;
    for (auto e : registry_.view<ui::comp::DestroyFlag>()) {
        flagged.push_back(e);
    }

    auto ancestorFlagged = [&](entt::entity e) {
        ui::Entt ew{*this, e};
        auto parent = ew.get<ui::comp::Relationship>().getParent();
        while (parent != entt::null) {
            if (!registry_.valid(parent)) {
                return true;
            }
            if (registry_.all_of<ui::comp::DestroyFlag>(parent)) {
                return true;
            }
            parent = registry_.get<ui::comp::Relationship>(parent).getParent();
        }
        return false;
    };

    for (auto e : flagged) {
        if (!registry_.valid(e) || !registry_.all_of<ui::comp::DestroyFlag>(e) || ancestorFlagged(e)) {
            continue;
        }

        ui::Entt ew{*this, e};
        auto parent = ew.get<ui::comp::Relationship>().getParent();
        if (parent == entt::null) {
            clear();
            return;
        }

        ui::Entt parent_ew{*this, parent};
        parent_ew.get<ui::comp::Relationship>().destroyChild(parent_ew, e);
    }
}
}  // namespace mle
