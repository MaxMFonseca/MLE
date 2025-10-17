#pragma once

#include "../Types.h"

namespace mle::ui {
/// SBO, cuz most containers will have 6 or less children
class EntityStorage {
  public:
    // 6 looks like a good number, can be adjusted later
    static constexpr usize MAX_ARRAY_SIZE = 6;

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
    [[nodiscard]] bool isArray() const { return count_ <= MAX_ARRAY_SIZE; }
    void insertInVec(Entry child, usize pos);

  private:
    union Storage {
        std::array<Entry, MAX_ARRAY_SIZE> array{};
        std::vector<Entry> vec;

        MLE_NO_COPY_MOVE(Storage)

        Storage() {};
        ~Storage() {};
    } children_;

    // This is used to track if array or vector, since we do not demote this is invalid after promotion
    // use size() and isArray() always
    usize count_ = 0;
};

}  // namespace mle::ui
