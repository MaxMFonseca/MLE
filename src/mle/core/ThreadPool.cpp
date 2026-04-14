#include "ThreadPool.h"

#include "mle/core/Logger.h"

namespace mle {
void ThreadPool::workerLoop() {
    while (!stop_.load(std::memory_order_relaxed)) {
        auto task_opt = task_queue_.popMove(true);
        if (task_opt && !stop_.load(std::memory_order_relaxed)) {
            (*task_opt)();
        }
    }
}

void ThreadPool::init(usize thread_count) {
    MLE_I("Initializing ThreadPool with {} threads", thread_count);

    stop_.store(false, std::memory_order_relaxed);
    for (usize i = 0; i < thread_count; ++i) {
        threads_.emplace_back([this] { workerLoop(); });
    }
}

void ThreadPool::shutdown() {
    MLE_I("Shutting down the ThreadPool");

    stop_.store(true, std::memory_order_relaxed);
    for (usize i = 0; i < threads_.size(); ++i) {
        task_queue_.push([] {});
    }
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads_.clear();
}
}  // namespace mle
