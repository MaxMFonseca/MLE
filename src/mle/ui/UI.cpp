#include "UI.h"

namespace mle {
void UI::setRoot(const std::string& element_name) {
    MLE_I("Setting UI root to element '{}'", element_name);

    auto root_table = getTableFor(element_name);

    registry_.clear();

    root_ = lua_element_ops_.createElement(root_table, entt::null);
};

sol::table UI::getTableFor(const std::string& element_name) {
    return lua_.require(element_name);
}

}  // namespace mle
