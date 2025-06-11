#include "Base.h"

#include <algorithm>

#include "mle/common/Assert.h"
#include "mle/common/Utils.h"
#include "mle/lua/Utils.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/LuaKeyHandlers.h"
#include "sol/forward.hpp"

namespace mle::ui::element {
void applyTable(entt::entity e, const sol::table& table, entt::entity parent) {
    auto& reg = getRegistry();
    if (parent != entt::null) {
        reg.emplace_or_replace<comp::Parent>(e, parent);
    }

    std::string name;
    if (lua::tryGetIdx(table, 1, name)) {
        reg.emplace_or_replace<comp::Name>(e, std::move(name));
    }

    auto* bounds_c = reg.try_get<comp::Bounds>(e);
    if (!bounds_c) {
        bounds_c = &reg.emplace<comp::Bounds>(e);
    }
    const auto bounds_r = table["bounds"];
    if (bounds_r.valid()) {
        bounds_c->immutable = true;
        bounds_c->bounds = lua::as<Recti>(bounds_r);
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
        fn_r.value()(e, value);
    }
}

bool isFitX(entt::entity e) {
    if (getRegistry().get<comp::Bounds>(e).immutable) {
        return false;
    }
    const auto* target_size = getRegistry().try_get<comp::TargetSize>(e);
    if (target_size) {
        return target_size->x.type == comp::TargetBound::Type::FIT;
    }
    return true;
}

bool isFitY(entt::entity e) {
    if (getRegistry().get<comp::Bounds>(e).immutable) {
        return false;
    }
    const auto* target_size = getRegistry().try_get<comp::TargetSize>(e);
    if (target_size) {
        return target_size->y.type == comp::TargetBound::Type::FIT;
    }
    return true;
}

bool isFit(entt::entity e) {
    if (getRegistry().get<comp::Bounds>(e).immutable) {
        return false;
    }
    const auto* target_size = getRegistry().try_get<comp::TargetSize>(e);
    if (target_size) {
        return target_size->x.type == comp::TargetBound::Type::FIT || target_size->y.type == comp::TargetBound::Type::FIT;
    }
    return true;
}

// entt::entity findFirstFixedSizedX(entt::entity e) {  // NOLINT
//     if (has<comp::StaticBounds>(e)) {
//         return e;
//     }
//     if (auto* target_size = getRegistry().try_get<comp::TargetSize>(e)) {
//         if (!target_size->isFitX()) {
//             return e;
//         }
//     }
//     return findFirstFixedSizedX(getRegistry().get<comp::Parent>(e).parent);
// }
//
// entt::entity findFirstFixedSizedY(entt::entity e) {  // NOLINT
//     if (has<comp::StaticBounds>(e)) {
//         return e;
//     }
//     if (auto* target_size = getRegistry().try_get<comp::TargetSize>(e)) {
//         if (!target_size->isFitY()) {
//             return e;
//         }
//     }
//     return findFirstFixedSizedY(getRegistry().get<comp::Parent>(e).parent);
// }
//

namespace comp {
void TargetBound::fromLua(const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<f32>()) {
        val = obj.as<f32>();
        return;
    }
    if (obj.is<std::string>()) {
        auto split = splitNumberAndSuffix<f32>(obj.as<std::string>());
        MLE_ASSERT_LOG(split.has_value(), "Invalid target bound format: {}", obj.as<std::string>());
        val = split.value().first;
        const auto& suffix = split.value().second;
        if (suffix == "px") {
            type = Type::PX;
        } else if (suffix == "%") {
            type = Type::PARENT;
        } else if (suffix == "fit") {
            type = Type::FIT;
        } else if (suffix == "s") {
            type = Type::SELF;
        } else if (suffix == "sw") {
            type = Type::SELF_W;
        } else if (suffix == "sh") {
            type = Type::SELF_H;
        } else if (suffix == "ppx") {
            type = Type::PARENT_PX;
        } else if (suffix == "r%") {
            type = Type::ROOT;
        } else if (suffix == "rpx") {
            type = Type::ROOT_PX;
        } else if (suffix == "flex") {
            type = Type::FLEX_SHARE;
        } else if (suffix == "pw") {
            type = Type::PARENT_W;
        } else if (suffix == "ph") {
            type = Type::PARENT_H;
        } else {
            type = Type::DEFAULT;
        }
        return;
    }
}
}  // namespace comp
}  // namespace mle::ui::element
