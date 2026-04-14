#include "SyncManager.h"

#include <algorithm>

#include "Renderer.h"
#include "mle/core/Logger.h"
#include "mle/core/ThreadPool.h"

namespace mle {
Semaphore& Semaphore::operator=(Semaphore&& rhs) {
    if (this != &rhs) {
        o_ = rhs.o_;
        rhs.o_ = nullptr;
    }
    return *this;
}

Semaphore Semaphore::create() {
    return Renderer::i().syncMgr().acquireSemaphore();
}

void Semaphore::release() {
    if (o_) {
        Renderer::i().syncMgr().reclaimSemaphore(o_);
        o_ = nullptr;
    }
}

Fence& Fence::operator=(Fence&& rhs) {
    if (this != &rhs) {
        o_ = rhs.o_;
        rhs.o_ = nullptr;
    }
    return *this;
}

Fence Fence::create() {
    return Renderer::i().syncMgr().acquireFence();
}

void Fence::release() {
    if (o_) {
        Renderer::i().syncMgr().reclaimFence(o_);
        o_ = nullptr;
    }
}

Result Fence::wait(u32 timeout_ms) const {
    auto fence_wait_result = Renderer::i().vk().dev().waitForFences(o_, vk::True, timeout_ms);
    if (fence_wait_result == vk::Result::eSuccess) {
        return Result::OK;
    }
    if (fence_wait_result == vk::Result::eTimeout) {
        return Result::TIMEOUT;
    }

    check(fence_wait_result);
    return Result::NOK;
}

void RendererSyncManager::init() {
    startWaiter();
}

void RendererSyncManager::shutdown() {
    stopWaiter();

    {
        std::scoped_lock lock(free_mutex_);
        for (auto& f : free_fences_) {
            Renderer::i().vk().destroy(f);
        }
        free_fences_.clear();

        for (auto& s : free_semaphores_) {
            Renderer::i().vk().destroy(s);
        }
        free_semaphores_.clear();
    }
}

Fence RendererSyncManager::acquireFence(bool signaled) {
    std::scoped_lock lock(free_mutex_);
    if (!free_fences_.empty()) {
        vk::Fence f = free_fences_.back();
        free_fences_.pop_back();
        return Fence(f);
    }

    vk::FenceCreateInfo fci{};
    if (signaled) {
        fci.flags = vk::FenceCreateFlagBits::eSignaled;
    }
    vk::Fence f = unwrap(Renderer::i().vk().dev().createFence(fci));
    return Fence(f);
}

Semaphore RendererSyncManager::acquireSemaphore() {
    std::scoped_lock lock(free_mutex_);
    if (!free_semaphores_.empty()) {
        vk::Semaphore s = free_semaphores_.back();
        free_semaphores_.pop_back();
        return Semaphore(s);
    }

    vk::SemaphoreCreateInfo sci{};
    vk::Semaphore s = unwrap(Renderer::i().vk().dev().createSemaphore(sci));
    return Semaphore(s);
}

void RendererSyncManager::reclaimFence(vk::Fence fence) {
    std::scoped_lock lock(free_mutex_);
    check(Renderer::i().vk().getDevice().resetFences(1, &fence));
    free_fences_.push_back(fence);
}

void RendererSyncManager::reclaimSemaphore(vk::Semaphore semaphore) {
    std::scoped_lock lock(free_mutex_);
    free_semaphores_.push_back(semaphore);
}

void RendererSyncManager::trackFence(Fence&& fence, std::move_only_function<void()>&& on_signal) {
    if (Renderer::i().vk().dev().getFenceStatus(fence.get()) == vk::Result::eSuccess) {
        reclaimFence(fence.get());
        on_signal();
        return;
    }

    TrackedFence t{std::move(fence), std::move(on_signal)};
    {
        std::scoped_lock lock(tracked_fences_mutex_);
        tracked_fences_.emplace_back(std::move(t));
    }
    waiter_cv_.notify_one();
}

void RendererSyncManager::stopWaiter() {
    if (!waiter_thread_.joinable()) {
        return;
    }

    waiter_thread_.request_stop();
    waiter_cv_.notify_all();
    waiter_thread_.join();

    std::scoped_lock lock(tracked_fences_mutex_);
    if (!tracked_fences_.empty()) {
        MLE_W("Some tracked fences are still pending on shutdown.");
    }
    tracked_fences_.clear();
}

void RendererSyncManager::startWaiter() {
    if (waiter_thread_.joinable()) {
        return;
    }
    waiter_thread_ = std::jthread([this](std::stop_token st) { waiterLoop(std::move(st)); });
}

void RendererSyncManager::waiterLoop(std::stop_token st) {
    std::vector<vk::Fence> fences;
    std::vector<usize> signaled_idx;

    while (!st.stop_requested()) {
        {
            std::unique_lock lock(tracked_fences_mutex_);
            waiter_cv_.wait(lock, [this, &st] { return st.stop_requested() || !tracked_fences_.empty(); });
            if (st.stop_requested()) {
                break;
            }
        }

        fences.clear();
        {
            std::scoped_lock lock(tracked_fences_mutex_);
            fences.reserve(tracked_fences_.size());
            for (auto& t : tracked_fences_) {
                fences.push_back(t.o.get());
            }
        }
        if (fences.empty()) {
            continue;
        }

        auto device = Renderer::i().vk().getDevice();

        check(device.waitForFences(fences, vk::False, UINT64_MAX));

        signaled_idx.clear();
        {
            std::scoped_lock lk(tracked_fences_mutex_);
            for (usize i = 0; i < tracked_fences_.size(); ++i) {
                if (device.getFenceStatus(tracked_fences_[i].o.get()) == vk::Result::eSuccess) {
                    signaled_idx.push_back(i);
                }
            }
            if (signaled_idx.empty()) {
                continue;
            }
        }

        struct DoneItem {
            std::move_only_function<void()> cb;
            Fence o;
        };
        std::vector<DoneItem> done;
        {
            std::scoped_lock lock(tracked_fences_mutex_);
            std::ranges::sort(signaled_idx, std::greater<>());
            done.reserve(signaled_idx.size());
            for (usize idx : signaled_idx) {
                done.emplace_back(std::move(tracked_fences_[idx].cb), std::move(tracked_fences_[idx].o));
                tracked_fences_.erase(tracked_fences_.begin() + as<isize>(idx));
            }
        }

        for (auto& d : done) {
            ThreadPool::i().enqueue(std::move(d.cb));
        }
    }
}

}  // namespace mle
