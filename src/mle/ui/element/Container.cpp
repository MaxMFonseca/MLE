#include "Container.h"

namespace mle::ui::element {
Container::~Container() {
    if (!isArray()) {
        children_.vec.~vector();
    }
}

[[nodiscard]] usize Container::size() const {
    if (isArray()) {
        return count_;
    }
    return children_.vec.size();
}
}  // namespace mle::ui::element
