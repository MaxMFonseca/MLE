#pragma once

#include "Types.h"
#include "mle/common/Utils.h"

namespace mle::ui {
class EntityStorage {
  public:
    MLE_NO_COPY_MOVE(EntityStorage)

    EntityStorage() = default;
    ~EntityStorage();

    [[nodiscard]] bool isArray() const { return count_ <= 6; }
    [[nodiscard]] std::span<const entt::entity> get() const;
    void add(entt::entity e, usize pos = max<usize>());
    void remove(entt::entity e);
    void getIdx(entt::entity e) const;
    [[nodiscard]] usize size() const;

  private:
    void insertInVec(entt::entity child_entt, usize pos = max<usize>());

  private:
    // Some sort of SBO, cuz most containers will have 6 or less children
    // TODO: 6 is used because sizeof(array) == sizeof(vec), but this does not really matter
    // Maybe if we see that most containers have like 10 children we can increase this number
    // allow for configuration
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
}  // namespace mle::ui
