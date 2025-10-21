#include "Types.h"

namespace mle::ui {
[[nodiscard]] vec2i PaddingPx::removeFrom(vec2i size) const {
    size.x -= l + r;
    size.y -= t + b;
    return size;
}

}  // namespace mle::ui
