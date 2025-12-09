#pragma once

#include "mle/math/Types.h"
#include "mle/utils/Utils.h"

namespace mle {
struct Vec2iHash {
    std::size_t operator()(const vec2i& p) const {
        auto h = as<std::size_t>(p.x);
        h = (h * 0x1f1f1f1f) ^ as<std::size_t>(p.y + 0x9e3779b9 + (h << 6U) + (h >> 2U));
        return h;
    }
};
}  // namespace mle
