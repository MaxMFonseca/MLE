#pragma once

#include "mle/core/Core.h"

namespace mle {
template <typename T>
[[nodiscard]] T unwrap(Expected<T>&& e) {
    if (!e) {
        core::unrecoverable("Unwrapping an error: {}", e.error());
    }
    return std::move(e).value();
}
}  // namespace mle
