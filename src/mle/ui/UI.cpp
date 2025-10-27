#include "UI.h"

#include "Entt.h"
#include "mle/core/PerfTracker.h"
#include "mle/lua/Utils.h"
#include "mle/ui/components/Bounds.h"
#include "mle/window/Window.h"

namespace mle {
UI::UI() {
    window_resize_el_ = Window::i().getED().makeListener<window::ev::Resize>([this](const window::ev::Resize& ev) { resizeRoot(ev.size); });
    lua_.init();
}

void UI::clear() {
    MLE_I("Clearing UI");
    registry_.clear();
    root_ = entt::null;
    root_size_ = vec2u{0};
    rendering_system_.clear();
}

void UI::setRoot(sol::table root_table) {
    MLE_I("Setting UI root from table: {}", root_table);

    addRootStyles(root_table["styles"]);

    registry_.clear();

    root_ = registry_.create();
    ui::Entt root_entt{*this, root_};
    root_entt.emplace<ui::comp::Relationship>();
    root_entt.applyTable(root_table);
    if (!root_entt.has<ui::comp::Container>()) {
        root_entt.emplace<ui::comp::Container>();
    }

    resizeRoot(Window::i().getSize());
};

void UI::resizeRoot(const vec2u& size) {
    if (root_ == entt::null) {
        return;
    }
    ui::Entt ew(*this, root_);
    ew.emplaceOrReplace<ui::comp::Bounds>(size);
    ew.addFlag<ui::comp::RequestInternalBoundsUpdateFlag>();
    root_size_ = size;
    root_aspect_ratio_ = as<f32>(size.x) / as<f32>(size.y);
};

void UI::setRoot(const std::string& element_name) {
    auto root_table = getTableFor(element_name);
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

sol::table UI::getTableFor(const std::string& element_name) {
    return lua_.require(element_name);
}

void UI::update() {
    MLE_PERF_SCOPE("GUIUpdate");

    {
        MLE_PERF_SCOPE("GUIUpdate.Hover");
        hover_system_.update();
    }

    {
        MLE_PERF_SCOPE("GUIUpdate.Bounds");
        bounds_system_.update();
    }

    {
        MLE_PERF_SCOPE("GUIUpdate.Rendering");
        rendering_system_.update();
    }
}

ImageRef UI::render() {
    MLE_PERF_SCOPE("GUIRender");
    return rendering_system_.render();
};

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
}  // namespace mle
