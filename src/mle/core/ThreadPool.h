#pragma once

#include <future>

#include "mle/utils/Types.h"
#include "mle/utils/Utils.h"
#include "mle/utils/containers/TSQueue.h"

// Very simple thread pool implementation. Should be ok for now, but I want to add more features, like prio.
namespace mle {
class ThreadPool {
    MLE_SINGLETON(ThreadPool)
  public:
    void init(usize thread_count = std::thread::hardware_concurrency());
    void shutdown();

    template <typename Func>
    void enqueue(Func&& func) {
        task_queue_.push(std::move_only_function<void()>(std::forward<Func>(func)));
    }

    template <typename Func>
    [[nodiscard]] auto submit(Func&& func) -> std::future<decltype(func())> {  // NOLINT
        using Ret = decltype(func());

        auto task = std::make_shared<std::packaged_task<Ret()>>(std::forward<Func>(func));
        std::future<Ret> result = task->get_future();

        task_queue_.push([task]() { (*task)(); });

        return result;
    }

    template <typename Func, typename Callback>
    void submitWithCallback(Func&& func, Callback&& callback) {
        using Ret = decltype(func());
        auto task = std::make_shared<std::packaged_task<Ret()>>(std::forward<Func>(func));
        std::future<Ret> future = task->get_future();

        task_queue_.push([task, cb = std::forward<Callback>(callback), future = std::move(future)]() mutable {
            (*task)();
            if constexpr (std::is_void_v<Ret>) {
                cb();
            } else {
                cb(future.get());
            }
        });
    }

    [[nodiscard]] usize threadCount() const { return threads_.size(); }

  private:
    void workerLoop();

    std::vector<std::thread> threads_;
    std::atomic<bool> stop_ = false;
    TSQueue<std::move_only_function<void()>> task_queue_;
};
}  // namespace mle
