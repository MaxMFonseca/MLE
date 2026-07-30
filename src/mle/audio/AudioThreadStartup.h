#pragma once

#include <condition_variable>
#include <mutex>

#include "mle/core/Result.h"

namespace mle::audio {
class AudioThreadStartup {
  public:
    void reset() {
        std::scoped_lock lock(mutex_);
        complete_ = false;
        result_ = Result::NOT_READY;
    }

    void publish(Result result) {
        {
            std::scoped_lock lock(mutex_);
            result_ = result;
            complete_ = true;
        }
        ready_.notify_one();
    }

    [[nodiscard]] Result wait() {
        std::unique_lock lock(mutex_);
        ready_.wait(lock, [this] { return complete_; });
        return result_;
    }

  private:
    std::mutex mutex_;
    std::condition_variable ready_;
    bool complete_{false};
    Result result_{Result::NOT_READY};
};
}  // namespace mle::audio
