#include "Element.h"

namespace mle::ui::detail::e_components {
Children::~Children() {
    if (!isArray()) {
        children_.vec.~vector();
    }
}

[[nodiscard]] uint Children::size() const {
    if (isArray()) {
        return count_;
    }
    return children_.vec.size();
}

void Children::add(entt::entity child) {
    if (isArray()) {
        if (count_ < 6) {
            children_.array.at(count_++) = child;
        } else {
            MLE_C("Promoted!");

            std::vector<entt::entity> temp(children_.array.begin(), children_.array.end());
            temp.push_back(child);

            new (&children_.vec) std::vector<entt::entity>(std::move(temp));
            count_++;
        }
    } else {
        children_.vec.push_back(child);
    }
}

void Children::remove(entt::entity child) {
    if (isArray()) {
        for (int i = 0; i < count_; ++i) {
            if (children_.array.at(i) == child) {
                for (int j = i; j < count_ - 1; ++j) {
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
    }
}
}  // namespace mle::ui::detail::e_components
