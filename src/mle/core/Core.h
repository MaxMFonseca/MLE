#pragma once

#include <string>

#include "Result.h"

namespace mle::core {
struct InitInfo {
    int argc = 0;
    char** argv = nullptr;
};

void init(const InitInfo& ii);

void unrecoverable(const std::string& msg);

template <typename... Args>
void unrecoverable(fmt::format_string<Args...> fmt_str, Args&&... args) {
    unrecoverable(fmt::format(fmt_str, std::forward<Args>(args)...));
}

template <typename T>
[[nodiscard]] T unwrap(Expected<T>&& e) {
    if (!e) {
        core::unrecoverable("Unwrapping an error: {}", e.error());
    }
    return std::move(e).value();
}
}  // namespace mle::core
