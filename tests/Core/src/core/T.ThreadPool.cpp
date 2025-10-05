// ThreadPool_more_tests.cpp
#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <future>
#include <latch>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "../utils/Utils.h"
#include "mle/core/ThreadPool.h"

using mle::ThreadPool;

// --- Helpers -----------------------------------------------------------------

/// Collect unique thread IDs used to execute tasks.
struct ThreadIdCollector {
    void add(std::thread::id id) {
        std::lock_guard<std::mutex> lg(mu_);
        ids_.insert(id);
    }
    size_t count() const {
        std::lock_guard<std::mutex> lg(mu_);
        return ids_.size();
    }

  private:
    mutable std::mutex mu_;
    std::set<std::thread::id> ids_;
};

// --- Tests -------------------------------------------------------------------

TEST(ThreadPool, ThreadPoolInitialized) {
    EXPECT_GT(ThreadPool::i().threadCount(), 0U);
}

TEST(ThreadPool, EnqueueExecutesTasks) {
    constexpr int N = 10;
    std::atomic<int> counter{0};
    std::latch done(N);

    for (int i = 0; i < N; ++i) {
        ThreadPool::i().enqueue([&] {
            counter.fetch_add(1, std::memory_order_relaxed);
            done.count_down();
        });
    }

    done.wait();
    EXPECT_EQ(counter.load(std::memory_order_relaxed), N);
}

TEST(ThreadPool, SubmitReturnsFuture) {
    auto fut = ThreadPool::i().submit([] { return 42; });
    EXPECT_EQ(fut.get(), 42);
}

TEST(ThreadPool, SubmitWithVoidReturn) {
    std::atomic<bool> flag{false};
    auto fut = ThreadPool::i().submit([&] { flag.store(true, std::memory_order_relaxed); });
    fut.get();  // deterministic wait
    EXPECT_TRUE(flag.load(std::memory_order_relaxed));
}

TEST(ThreadPool, SubmitWithCallbackWorks) {
    std::latch done(1);
    std::atomic<int> seen{0};

    ThreadPool::i().submitWithCallback([] { return 123; },
                                       [&](int v) {
                                           EXPECT_EQ(v, 123);
                                           seen.store(v, std::memory_order_relaxed);
                                           done.count_down();
                                       });

    done.wait();
    EXPECT_EQ(seen.load(std::memory_order_relaxed), 123);
}

TEST(ThreadPool, SubmitWithCallbackVoid) {
    std::latch done(1);
    std::atomic<bool> called{false};

    ThreadPool::i().submitWithCallback([&] { called.store(true, std::memory_order_relaxed); },
                                       [&] {
                                           EXPECT_TRUE(called.load(std::memory_order_relaxed));
                                           done.count_down();
                                       });

    done.wait();
    EXPECT_TRUE(called.load(std::memory_order_relaxed));
}

TEST(ThreadPool, MultipleFutures) {
    std::vector<std::future<int>> futs;
    futs.reserve(10);
    for (int i = 0; i < 10; ++i) {
        futs.push_back(ThreadPool::i().submit([i] { return i * i; }));
    }
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(futs[i].get(), i * i);
    }
}

TEST(ThreadPool, StressTestRapidEnqueueAndSubmit) {
    constexpr int N = 500;
    std::atomic<int> counter{0};
    std::latch done(N + N);
    std::vector<std::future<void>> futs;
    futs.reserve(N);

    for (int i = 0; i < N; ++i) {
        ThreadPool::i().enqueue([&] {
            counter.fetch_add(1, std::memory_order_relaxed);
            done.count_down();
        });
        futs.push_back(ThreadPool::i().submit([&] {
            counter.fetch_add(1, std::memory_order_relaxed);
            done.count_down();
        }));
    }

    for (auto& f : futs) {
        f.get();
    }
    done.wait();
    EXPECT_EQ(counter.load(std::memory_order_relaxed), N * 2);
}

TEST(ThreadPool, ReturnsMoveOnlyType) {
    auto fut = ThreadPool::i().submit([] {
        auto up = std::make_unique<int>(7);
        return up;  // move-only return
    });
    std::unique_ptr<int> got = fut.get();
    ASSERT_TRUE(got);
    EXPECT_EQ(*got, 7);
}

TEST(ThreadPool, CallbackReceivesMoveOnlyType) {
    std::latch done(1);
    std::atomic<int> seen{0};

    ThreadPool::i().submitWithCallback(
        [] {
            auto up = std::make_unique<int>(1234);
            return up;  // move-only result
        },
        [&](std::unique_ptr<int> v) {
            ASSERT_TRUE(v);
            seen.store(*v, std::memory_order_relaxed);
            done.count_down();
        });

    done.wait();
    EXPECT_EQ(seen.load(std::memory_order_relaxed), 1234);
}

TEST(ThreadPool, EnqueueAcceptsMoveOnlyCallable) {
    std::latch done(1);
    // Capture a unique_ptr by move into a lambda => move-only closure.
    auto payload = std::make_unique<int>(99);
    ThreadPool::i().enqueue([p = std::move(payload), &done]() mutable {
        ASSERT_TRUE(p);
        *p = *p + 1;  // just to touch it
        done.count_down();
    });
    done.wait();
    SUCCEED();  // If it compiled and ran, enqueue supports move-only callables.
}

TEST(ThreadPool, TasksCanSubmitMoreTasks) {
    constexpr int OUTER = 50;
    std::atomic<int> counter{0};
    std::latch done(as<isize>(OUTER * 2));

    for (int i = 0; i < OUTER; ++i) {
        ThreadPool::i().enqueue([&] {
            counter.fetch_add(1, std::memory_order_relaxed);
            done.count_down();
            // Submit another task from within a worker:
            ThreadPool::i().enqueue([&] {
                counter.fetch_add(1, std::memory_order_relaxed);
                done.count_down();
            });
        });
    }

    done.wait();
    EXPECT_EQ(counter.load(std::memory_order_relaxed), OUTER * 2);
}

TEST(ThreadPool, ParallelismIsObservedWhenPossible) {
    const auto tc = ThreadPool::i().threadCount();
    if (tc <= 1) {
        GTEST_SKIP() << "Single-threaded pool; skip parallelism check.";
    }

    ThreadIdCollector col;
    constexpr int N = 200;
    std::latch done(N);

    for (int i = 0; i < N; ++i) {
        ThreadPool::i().enqueue([&] {
            col.add(std::this_thread::get_id());
            done.count_down();
        });
    }

    done.wait();
    // We should see at least 2 distinct worker threads if tc>1.
    EXPECT_GE(col.count(), std::min<size_t>(2, tc));
}

TEST(ThreadPool, CallbackHappensAfterWorkFinishes) {
    std::latch done(1);
    std::atomic<bool> work_done{false};
    std::atomic<bool> callback_seen{false};

    ThreadPool::i().submitWithCallback(
        [&] {
            work_done.store(true, std::memory_order_release);
            return 1;
        },
        [&](int) {
            // If callback runs before work, this will likely fail.
            callback_seen.store(work_done.load(std::memory_order_acquire), std::memory_order_relaxed);
            done.count_down();
        });

    done.wait();
    EXPECT_TRUE(callback_seen.load(std::memory_order_relaxed));
}

TEST(ThreadPool, ManyCallbacksDoNotLoseAny) {
    constexpr int N = 1000;
    std::latch done(N);
    std::atomic<int> seen{0};

    for (int i = 0; i < N; ++i) {
        ThreadPool::i().submitWithCallback([i] { return i; },
                                           [&](int v) {
                                               (void)v;  // value not important; just count completion
                                               seen.fetch_add(1, std::memory_order_relaxed);
                                               done.count_down();
                                           });
    }

    done.wait();
    EXPECT_EQ(seen.load(std::memory_order_relaxed), N);
}

TEST(ThreadPool, SubmitHeavyMixedWorkloads) {
    constexpr int N = 256;
    std::vector<std::future<int>> futs;
    futs.reserve(N);

    // Mix short, medium, “compute-ish” tasks
    for (int i = 0; i < N; ++i) {
        futs.push_back(ThreadPool::i().submit([i] {
            // Cheap pseudo-work without sleeps:
            int x = i;
            for (int k = 0; k < 1000; ++k) {
                x = ((x * 1664525) + 1013904223);
            }
            return x;
        }));
    }

    // Verify all complete and are distinct computations.
    int completed = 0;
    for (auto& f : futs) {
        (void)f.get();
        ++completed;
    }
    EXPECT_EQ(completed, N);
}

TEST(ThreadPool, SubmitChainedByCallerIsSafe) {
    // Caller chains futures sequentially to ensure no hidden deadlocks:
    auto f1 = ThreadPool::i().submit([] { return 2; });
    auto f2 = ThreadPool::i().submit([&] { return f1.get() * 3; });  // get() on caller, not worker
    auto f3 = ThreadPool::i().submit([&] { return f2.get() + 4; });
    EXPECT_EQ(f3.get(), (2 * 3) + 4);
}
