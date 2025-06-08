#include "ElementManager.h"

#include "Element.h"
#include "mle/lua/Utils.h"

namespace mle::ui::detail {
void ElementManager::reset(sol::table table) {
    MLE_D("Resetting ElementManager");
    registry_.clear();

    if (!table.valid()) {
        MLE_I("ElementManager reset called with no table");
        return;
    }

    MLE_D("Root element: {}", table);

    root_ = registry_.create();
    registry_.emplace<e_components::Base>(root_, "root", entt::null);
    auto& children = registry_.emplace<e_components::Children>(root_);

    sol::table flex = table["flex"];
    for (auto& child : flex) {
        auto table = lua::as<sol::table>(child.second);
        std::string child_name;
        MLE_ASSERT(lua::tryGetKeyOrIdx(table, "name", 1, child_name));
        entt::entity child_entity = registry_.create();
        registry_.emplace<e_components::Base>(child_entity, child_name, root_);
        children.add(child_entity);
        MLE_D("Added child element: {}", child_name);
        MLE_VD(children.size());
    }
}

void ElementManager::render() {
    auto root_children = registry_.get<e_components::Children>(root_).get();
    for (const auto& child : root_children) {
        auto name = registry_.get<e_components::Base>(child).name;
        MLE_D("Rendering child element: {}", name);
    }
}
}  // namespace mle::ui::detail
