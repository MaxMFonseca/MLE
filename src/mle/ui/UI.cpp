#include "UI.h"

#include "Entt.h"
#include "mle/ui/components/Bounds.h"
#include "mle/window/Window.h"

namespace mle {
void UI::setRoot(const std::string& element_name) {
    MLE_I("Setting UI root to element '{}'", element_name);

    auto root_table = getTableFor(element_name);

    registry_.clear();

    root_ = lua_element_ops_.createElement(root_table, entt::null);

    resizeRoot(Window::i().getSize());
};

void UI::resizeRoot(const vec2u& size) {
    ui::Entt e(*this, root_);
    e.emplaceOrReplace<ui::comp::Bounds>(size);
    e.addFlag<ui::comp::ContainerNeedsInternalBoundsUpdateFlag>();
    root_size_ = size;
    root_aspect_ratio_ = static_cast<f32>(size.x) / static_cast<f32>(size.y);
};

sol::table UI::getTableFor(const std::string& element_name) {
    return lua_.require(element_name);
}

}  // namespace mle
