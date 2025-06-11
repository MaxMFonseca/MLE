#include "Base.h"

#include "mle/common/Assert.h"
#include "mle/lua/Utils.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/LuaKeyHandlers.h"

namespace mle::ui::element {
void applyTable(entt::entity entity, const sol::table& table, entt::entity parent) {
    auto& reg = getRegistry();
    if (parent != entt::null) {
        reg.emplace_or_replace<comp::Parent>(entity, parent);
    }

    std::string name;
    if (lua::tryGetIdx(table, 1, name)) {
        reg.emplace_or_replace<comp::Name>(entity, std::move(name));
    }

    for (auto& [key, value] : table) {
        std::string key_str;
        if (!lua::tryAs(key, key_str)) {
            continue;
        }
        auto fn_r = getLuaKeyHandlerFn(key_str);
        if (!fn_r) {
            continue;
        }
        fn_r.value()(entity, value);
    }
}
}  // namespace mle::ui::element
