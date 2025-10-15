#pragma once

#include "../Types.h"

namespace mle::ui {
/// SBO, cuz most containers will have 6 or less children
class EntityStorage {
  public:
    struct Entry {
        std::string name;
        entt::entity e;
    };

  public:
    MLE_NO_COPY_MOVE(EntityStorage)

    EntityStorage() = default;
    ~EntityStorage();

    [[nodiscard]] std::span<const Entry> get() const;
    void add(std::string name, entt::entity e, usize pos = max<usize>());
    void remove(usize idx);
    void remove(entt::entity e);
    void remove(const std::string& name);
    [[nodiscard]] std::optional<usize> getIdx(entt::entity e) const;
    [[nodiscard]] std::optional<usize> getIdx(const std::string& name) const;
    [[nodiscard]] entt::entity getEFromName(const std::string& name) const;
    [[nodiscard]] std::string getNameFromE(entt::entity e) const;

    [[nodiscard]] usize size() const {
        if (isArray()) {
            return count_;
        }
        return children_.vec.size();
    };

  private:
    [[nodiscard]] bool isArray() const { return count_ <= 6; }
    void insertInVec(Entry child, usize pos);

  private:
    // TODO: 6 is used because sizeof(array) == sizeof(vec), but this does not really matter
    // Maybe if we see that most containers have like 10 children we can increase this number
    // allow for configuration
    union Storage {
        std::array<Entry, 6> array{};
        std::vector<Entry> vec;

        MLE_NO_COPY_MOVE(Storage)

        Storage() {};
        ~Storage() {};
    } children_;

    // This is used to track if array or vector, since we do not demote this is invalid after promotion
    // use size() and isArray() always
    usize count_ = 0;
};

namespace comp {
struct Container {
    EntityStorage o;
};
}  // namespace comp
}  // namespace mle::ui
