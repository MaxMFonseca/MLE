#include "Utils.h"

#include "mle/common/Assert.h"

namespace mle::ui {
EntityStorage::~EntityStorage() {
    if (!isArray()) {
        children_.vec.~vector();
    }
}

std::span<const entt::entity> EntityStorage::get() const {
    if (isArray()) {
        return {children_.array.data(), count_};
    }
    return {children_.vec.data(), children_.vec.size()};
}

usize EntityStorage::size() const {
    if (isArray()) {
        return count_;
    }
    return children_.vec.size();
}

void EntityStorage::add(entt::entity e, usize pos) {
    if (isArray()) {
        if (count_ < 6) {
            if (pos == max<usize>()) {
                children_.array.at(count_) = e;
            } else {
                for (usize i = count_; i > pos; --i) {
                    children_.array.at(i) = children_.array.at(i - 1);
                }
                children_.array.at(pos) = e;
            }
        } else {
            // promote to vector
            new (&children_.vec) std::vector<entt::entity>(children_.array.begin(), children_.array.end());

            insertInVec(e, pos);
        }
        count_++;
    } else {
        insertInVec(e, pos);
    }
}

void EntityStorage::insertInVec(entt::entity child_entt, usize pos) {
    MLE_ASSERT_LOG(!isArray(), "Cannot insert in vec when using array storage");
    if (pos == max<usize>()) {
        children_.vec.push_back(child_entt);
    } else {
        children_.vec.insert(children_.vec.begin() + as<uint>(pos), child_entt);
    }
}
}  // namespace mle::ui
