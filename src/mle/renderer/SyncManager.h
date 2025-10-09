#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

#include "Types.h"
#include "mle/utils/Utils.h"

namespace mle {
class RendererSyncManager;

class Semaphore {
  public:
    ~Semaphore() { release(); }

    Semaphore& operator=(Semaphore&& rhs);
    Semaphore(Semaphore&& rhs) { *this = std::move(rhs); }

    MLE_NO_COPY(Semaphore);

    [[nodiscard]] vk::Semaphore get() const { return o_; }

    static Semaphore create();

  private:
    void release();

  private:
    friend RendererSyncManager;
    explicit Semaphore(vk::Semaphore semaphore) :
        o_(semaphore) {}

    vk::Semaphore o_{};
};

class Fence {
  public:
    ~Fence() { release(); }

    MLE_NO_COPY(Fence);

    Fence& operator=(Fence&& rhs);
    Fence(Fence&& rhs) { *this = std::move(rhs); }

    [[nodiscard]] vk::Fence get() const { return o_; }

    Result wait(u32 timeout_ms = max<u32>()) const;

    static Fence create();

  private:
    void release();

  private:
    friend RendererSyncManager;
    explicit Fence(vk::Fence fence) :
        o_(fence) {}

    vk::Fence o_{};
};

class RendererSyncManager {
  public:
    MLE_NO_COPY_MOVE(RendererSyncManager)

    ~RendererSyncManager() = default;

    Fence acquireFence(bool signaled = false);
    Semaphore acquireSemaphore();

    void reclaimFence(vk::Fence fence);
    void reclaimSemaphore(vk::Semaphore semaphore);

    void trackFence(Fence&& fence, std::move_only_function<void()>&& on_signal);

  private:
    friend Renderer;
    RendererSyncManager() = default;
    void init();
    void shutdown();

    void startWaiter();
    void stopWaiter();
    void waiterLoop(std::stop_token st);

  private:
    std::mutex free_mutex_;
    std::vector<vk::Fence> free_fences_;
    std::vector<vk::Semaphore> free_semaphores_;

    struct TrackedFence {
        Fence o;
        std::move_only_function<void()> cb;

        TrackedFence(Fence&& fence, std::move_only_function<void()>&& on_signal) :
            o(std::move(fence)),
            cb(std::move(on_signal)) {}
    };

    std::deque<TrackedFence> tracked_fences_;
    std::mutex tracked_fences_mutex_;
    std::condition_variable waiter_cv_;
    std::jthread waiter_thread_;
    std::atomic<bool> waiter_running_{false};
};
}  // namespace mle
