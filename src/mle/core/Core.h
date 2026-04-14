#pragma once

#include <mutex>
#include <random>
#include <string>

#include "Result.h"
#include "mle/utils/Stopwatch.h"
#include "mle/utils/Utils.h"

namespace mle {

class Core final {
    MLE_SINGLETON(Core)

  public:
    struct InitInfo {
        int argc = 0;
        char** argv = nullptr;
    };

    void init(const InitInfo& ii);
    void shutdown();

    void unrecoverable(const std::string& msg);
    template <typename... Args>
    void unrecoverable(fmt::format_string<Args...> fmt_str, Args&&... args) {
        unrecoverable(fmt::format(fmt_str, std::forward<Args>(args)...));
    }

    template <typename... Args>
    static void check(bool v, fmt::format_string<Args...> fmt_str, Args&&... args) {
        if (!v) {
            Core::i().unrecoverable(std::forward<fmt::format_string<Args...>>(fmt_str), std::forward<Args>(args)...);
        }
    }

    template <typename... Args>
    void check(Result r, fmt::format_string<Args...> fmt_str, Args&&... args) {
        check(!isError(r), std::forward<fmt::format_string<Args...>>(fmt_str), std::forward<Args>(args)...);
    }

  private:
    Stopwatch running_time_{};
};

}  // namespace mle
