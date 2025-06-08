#pragma once

#include <entt/entt.hpp>

#include "mle/common/Utils.h"
#include "mle/common/math/Types.h"

namespace mle::ui::detail::e_components {  // NOLINT
struct Base {
    std::string name;
    entt::entity parent;
};
struct Position {
    vec2f pos;

    struct {
        vec2f pos;
    } target;
};
struct Padding {};
struct Margin {};
struct Size {};

class Children {
  public:
    MLE_NO_COPY_MOVE(Children)

    Children() = default;
    ~Children();

    [[nodiscard]] bool isArray() const { return count_ <= 6; }
    [[nodiscard]] uint size() const;
    void add(entt::entity child);
    void remove(entt::entity child);
    [[nodiscard]] std::span<const entt::entity> get() {
        if (isArray()) {
            return {children_.array.data(), count_};
        }
        return {children_.vec.data(), children_.vec.size()};
    }

  private:
    union Storage {
        std::array<entt::entity, 6> array{};
        std::vector<entt::entity> vec;

        MLE_NO_COPY_MOVE(Storage)

        Storage() {};
        ~Storage() {};
    } children_;
    // This is used to track if array or vector, since we do not demote this can become invalid after promotion
    u8 count_ = 0;
};

}  // namespace mle::ui::detail::e_components
