#include "Container.h"

#include "mle/lua/Utils.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/detail/Element.h"
#include "mle/ui/detail/ElementManager.h"

namespace mle::ui::detail::e_components {
Children::~Children() {
    if (!isArray()) {
        children_.vec.~vector();
    }
}

[[nodiscard]] usize Children::size() const {
    if (isArray()) {
        return count_;
    }
    return children_.vec.size();
}

void Children::addMany(const sol::table& table, entt::entity parent) {
    for (const auto& child : table) {
        add(child.second, parent);
    }
}

void Children::add(const sol::table& table, entt::entity parent) {
    auto& reg = ui::detail::getElementManager().getRegistry();
    auto e = reg.create();
    [[maybe_unused]] auto& base = reg.emplace<e_components::Base>(e, parent);

    if (auto name_r = table[1]; name_r.is<std::string>()) {
        reg.emplace<e_components::Name>(e, lua::as<std::string>(name_r));
    }

    for (auto& [key, value] : table) {
        std::string key_str;
        if (!lua::tryAs(key, key_str)) {
            continue;
        }
        auto fn_r = ui::detail::getElementManager().findLuaKey(key_str);
        if (!fn_r) {
            continue;
        }
        fn_r.value()(reg, e, value);
    }

    if (isArray()) {
        if (count_ < 6) {
            children_.array.at(count_++) = e;
        } else {
            std::vector<entt::entity> temp(children_.array.begin(), children_.array.end());
            temp.push_back(e);

            new (&children_.vec) std::vector<entt::entity>(std::move(temp));
            count_++;
        }
    } else {
        children_.vec.push_back(e);
    }
}

void Children::remove(entt::entity child) {
    if (isArray()) {
        for (usize i = 0; i < count_; ++i) {
            if (children_.array.at(i) == child) {
                for (usize j = i; j < count_ - 1; ++j) {
                    children_.array.at(j) = children_.array.at(j + 1);
                }
                --count_;
                return;
            }
        }
    } else {
        auto& vec = children_.vec;
        auto it = std::ranges::find(vec, child);
        if (it != vec.end()) {
            vec.erase(it);
        }
        // Do not decrement count_ for vector!
    }
}

void Children::render() const {  // NOLINT
    auto& reg = ui::detail::getElementManager().getRegistry();
    auto children = get();
    for (const auto& child : children) {
        // if (auto* const name = reg.try_get<e_components::Name>(child); name) {
        //     MLE_D("Named child: {}", name->name);
        // }
        // if (const auto* renderable = reg.try_get<e_components::Renderable>(child); renderable) {
        // }
        if (const auto* child_children = reg.try_get<e_components::Children>(child); child_children) {
            child_children->render();
        }
    }
}

void FlexContainer::onLuaKeyFlex(entt::registry& reg, entt::entity e, const sol::object& obj) {
    // auto& container =
    reg.get_or_emplace<FlexContainer>(e);
    auto& children = reg.get_or_emplace<Children>(e);
    children.addMany(obj, e);
}

}  // namespace mle::ui::detail::e_components
