#pragma once

#include "Types.h"
#include "math/Types.h"
#include "mle/common/Utils.h"

namespace std {
template <>
struct hash<mle::vec2i> {
    std::size_t operator()(const mle::vec2i& v) const noexcept { return (mle::as<std::size_t>(v.x) << 32U) | mle::as<std::size_t>(v.y); }
};
}  // namespace std
