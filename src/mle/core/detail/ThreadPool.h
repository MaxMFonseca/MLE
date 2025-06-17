#pragma once

#include "mle/common/Utils.h"
#include "mle/common/containers/TSQueue.h"

// Very simple thread pool implementation. Should be ok for now, but I want to add more features, like prio.
namespace mle::core::detail {
class ThreadPool {
  public:
    MLE_NO_COPY_MOVE(ThreadPool)

    ThreadPool() = default;
    ~ThreadPool() = default;

    void init(usize thread_count = std::thread::hardware_concurrency());
    void shutdown();

    template <typename Func>
    void enqueue(Func&& func) {
        task_queue_.push(std::function<void()>(std::forward<Func>(func)));
    }

  private:
    void workerLoop();

    std::vector<std::thread> threads_;
    std::atomic<bool> stop_ = false;
    TSQueue<std::function<void()>> task_queue_;
};
}  // namespace mle::core::detail
