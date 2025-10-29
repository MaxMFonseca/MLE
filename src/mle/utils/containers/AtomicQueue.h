#pragma once

#include <atomic>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "mle/core/Assert.h"
#include "mle/math/Utils.h"
#include "mle/utils/Types.h"
#include "mle/utils/Utils.h"

namespace mle {
template <typename T>
class AtomicQueue {
  public:
    explicit AtomicQueue(usize capacity) {
        usize cap = roundUpToPow2(capacity < 2 ? 2 : capacity);
        const usize bytes = cap * sizeof(T);
        void* mem = operator new(bytes, std::nothrow);
        if (!mem) {
            return;
        }

        storage_ = mem;
        capacity_ = cap;
        mask_ = cap - 1;
    }

    ~AtomicQueue() {
        clear();
        operator delete(storage_);
    }

    MLE_NO_COPY_MOVE(AtomicQueue)

    [[nodiscard]] usize capacity() const { return capacity_; }

    [[nodiscard]] usize size() const {
        const usize h = head_.load(std::memory_order_acquire);
        const usize t = tail_.load(std::memory_order_acquire);
        return t - h;
    }

    [[nodiscard]] bool empty() const { return size() == 0; }
    [[nodiscard]] bool full() const { return size() == capacity_; }

    void clear() {
        usize h = head_.load(std::memory_order_relaxed);
        const usize t = tail_.load(std::memory_order_acquire);
        while (h != t) {
            destroyAt(indexOf(h));
            ++h;
        }
        head_.store(h, std::memory_order_release);
    }

    bool tryPush(const T& v) { return emplaceImpl(v); }
    bool tryPush(T&& v) { return emplaceImpl(std::move(v)); }
    template <class... Args>
    bool tryEmplace(Args&&... args) {
        return emplaceImpl(std::forward<Args>(args)...);
    }

    bool tryPop(T& out) {
        usize h = head_.load(std::memory_order_relaxed);
        const usize t = tail_.load(std::memory_order_acquire);
        if (h == t) {
            return false;
        }
        T* ptr = ptrAt(indexOf(h));
        out = std::move(*ptr);
        ptr->~T();
        head_.store(h + 1, std::memory_order_release);
        return true;
    }

  private:
    [[nodiscard]] usize indexOf(usize i) const { return i & mask_; }

    [[nodiscard]] T* ptrAt(usize idx) {
        MLE_ASSERT(idx < capacity_);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
        return std::launder(rAs<T*>(as<u8*>(storage_) + (idx * sizeof(T))));
    }

    void destroyAt(usize idx) { ptrAt(idx)->~T(); }

    template <class... Args>
    bool emplaceImpl(Args&&... args) {
        usize t = tail_.load(std::memory_order_relaxed);
        const usize h = head_.load(std::memory_order_acquire);
        if ((t - h) == capacity_) {
            return false;
        }
        T* slot = ptrAt(indexOf(t));
        new (slot) T(std::forward<Args>(args)...);
        tail_.store(t + 1, std::memory_order_release);
        return true;
    }

  private:
    static constexpr auto CACHELINE = std::hardware_destructive_interference_size;
    static constexpr auto PAD_SIZE = CACHELINE - sizeof(std::atomic<usize>) > 0 ? CACHELINE - sizeof(std::atomic<usize>) : 1;

    alignas(CACHELINE) std::atomic<usize> head_{0};
    std::array<u8, PAD_SIZE> pad0_{};

    alignas(CACHELINE) std::atomic<usize> tail_{0};
    std::array<u8, PAD_SIZE> pad1_{};

    void* storage_ = nullptr;
    usize capacity_ = 0;
    usize mask_ = 0;
};
}  // namespace mle
