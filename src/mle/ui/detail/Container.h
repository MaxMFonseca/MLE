#pragma once

#include <entt/entt.hpp>

#include "mle/common/Utils.h"
#include "sol/forward.hpp"

namespace mle::ui::detail::e_components {
class Children {
  public:
    MLE_NO_COPY_MOVE(Children)

    Children() = default;
    ~Children();

    [[nodiscard]] bool isArray() const { return count_ <= 6; }
    [[nodiscard]] usize size() const;
    void addMany(const sol::table& table, entt::entity parent);
    void add(const sol::table& table, entt::entity parent);
    void remove(entt::entity child);
    [[nodiscard]] std::span<const entt::entity> get() const {
        if (isArray()) {
            return {children_.array.data(), count_};
        }
        return {children_.vec.data(), children_.vec.size()};
    }
    void render() const;

  private:
    union Storage {
        std::array<entt::entity, 6> array{};
        std::vector<entt::entity> vec;

        MLE_NO_COPY_MOVE(Storage)

        Storage() {};
        ~Storage() {};
    } children_;
    // This is used to track if array or vector, since we do not demote this can become invalid after promotion
    usize count_ = 0;
};

class FlexContainer {
  public:
    static void onLuaKeyFlex(entt::registry& reg, entt::entity e, const sol::object& obj);

  private:
};
}  // namespace mle::ui::detail::e_components
