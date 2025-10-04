#pragma once

#include <queue>

#include "mle/common/Utils.h"

namespace mle {
template <typename T>
class TSQueue {
  public:
    MLE_NO_COPY_MOVE(TSQueue)

    TSQueue() = default;
    ~TSQueue() = default;

    void push(const T& element) {
        std::scoped_lock lock(mutex_);
        queue_.push(element);
        cv_.notify_one();
    }

    void push(T&& element) {
        std::scoped_lock lock(mutex_);
        queue_.push(std::move(element));
        cv_.notify_one();
    }

    std::optional<T> pop(bool wait = true) {
        std::unique_lock lock(mutex_);

        if (queue_.empty() && wait) {
            cv_.wait(lock, [this] { return !queue_.empty(); });
        }
        if (queue_.empty()) {
            return std::nullopt;
        }

        auto ret = queue_.front();
        queue_.pop();
        return ret;
    }

    std::vector<T> popAll() {
        std::scoped_lock lock(mutex_);
        std::vector<T> ret;
        ret.reserve(queue_.size());
        while (!queue_.empty()) {
            ret.push_back(std::move(queue_.front()));
            queue_.pop();
        }
        return ret;
    }

    std::optional<T> popMove(bool wait = true) {
        std::unique_lock lock(mutex_);

        if (queue_.empty() && wait) {
            cv_.wait(lock, [this] { return !queue_.empty(); });
        }
        if (queue_.empty()) {
            return std::nullopt;
        }

        auto ret = std::move(queue_.front());
        queue_.pop();
        return ret;
    }

    void clear() {
        std::scoped_lock lock(mutex_);
        queue_.clear();
    }

    class LockedProxy {
      public:
        MLE_NO_COPY(LockedProxy)
        LockedProxy& operator=(LockedProxy&&) = delete;
        LockedProxy(LockedProxy&&) = default;

        LockedProxy(std::unique_lock<std::mutex> lock, std::queue<T>& queue) :
            lock_(std::move(lock)),
            queue_(queue) {}
        ~LockedProxy() = default;

        std::queue<T>& get() { return queue_; }

      private:
        std::unique_lock<std::mutex> lock_;
        std::queue<T>& queue_;
    };

    [[nodiscard]] LockedProxy access() { return LockedProxy(std::unique_lock<std::mutex>{mutex_}, queue_); }

  private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
};
}  // namespace mle
