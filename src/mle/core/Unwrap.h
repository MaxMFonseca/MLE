#pragma once

#include "Core.h"

namespace mle {
template <typename T>
[[nodiscard]] T unwrap(Expected<T>&& e) {
    if (!e) {
        Core::i().unrecoverable("Unwrapping an error: {}", e.error());
    }
    return std::move(e).value();
}

inline void check(Result res) {
    if (isError(res)) {
        Core::i().unrecoverable("Check error: {}", res);
    }
}
}  // namespace mle
