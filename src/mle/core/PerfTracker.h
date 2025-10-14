#pragma once

#include <chrono>
#include <deque>
#include <mutex>
#include <set>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include "mle/math/Types.h"
#include "mle/utils/Utils.h"

namespace mle {
class PerfNewSamplesListener;
using PerfNewSamplesListenerRef = PerfNewSamplesListener*;
using PerfNewSamplesListenerHnd = std::unique_ptr<PerfNewSamplesListener>;

class PerfTracker {
    MLE_SINGLETON(PerfTracker)

  public:
    using CounterId = u32;

    class Scoped {
      public:
        explicit Scoped(CounterId id) :
            id_(id),
            t0_(std::chrono::steady_clock::now()) {}
        ~Scoped() { PerfTracker::i().record(id_, t0_); }

        MLE_NO_COPY_MOVE(Scoped)

      private:
        CounterId id_;
        std::chrono::steady_clock::time_point t0_;
    };

    struct Sample {
        CounterId id_{};
        const char* name = nullptr;
        u64 last_sec_ns{0};
        f64 per_call_us{0.0};
        u64 calls_last_sec{0};
    };
    using Samples = std::vector<Sample>;

    using NewSamplesCallback = std::move_only_function<void(const Samples& samples)>;

  public:
    CounterId registerCounter(const char* name);
    void record(CounterId id, std::chrono::nanoseconds ns);
    void record(CounterId id, std::chrono::steady_clock::time_point start) { record(id, std::chrono::steady_clock::now() - start); }
    void record(const char* name, std::chrono::steady_clock::time_point start) { record(registerCounter(name), start); }

    void addNewSamplesListener(PerfNewSamplesListenerRef l);
    void removeNewSamplesListener(PerfNewSamplesListenerRef l);

  private:
    friend class Core;
    void init();
    void shutdown();

    void startUpdateThread();
    void stopUpdateThread();
    void update(std::stop_token st);

  private:
    struct Entry {
        u64 ns{0};
        u64 calls{0};
    };
    struct AtomicEntry {
        std::atomic<u64> ns{0};
        std::atomic<u64> calls{0};
    };

    const char* idToName(CounterId id);

    std::mutex names_mtx_;
    std::unordered_map<std::string, CounterId> name_to_id_;
    std::vector<std::string> id_to_name_;

    std::mutex entries_resize_mtx_;
    std::deque<AtomicEntry> entries_;

    std::vector<Entry> lifetime_acc_;

    std::chrono::steady_clock::time_point first_time_{std::chrono::steady_clock::now()};

    std::jthread update_thread_;

    std::mutex new_samples_listeners_mtx_;
    std::set<PerfNewSamplesListenerRef> new_samples_listeners_;
};

class PerfNewSamplesListener {
  public:
    using CbFn = PerfTracker::NewSamplesCallback;

  public:
    explicit PerfNewSamplesListener(CbFn&& cb) :
        cb_(std::move(cb)) {
        listen();
    }
    ~PerfNewSamplesListener() { unlisten(); }

    MLE_NO_COPY_MOVE(PerfNewSamplesListener);

    void listen() { PerfTracker::i().addNewSamplesListener(this); }
    void unlisten() { PerfTracker::i().removeNewSamplesListener(this); }

  private:
    friend class PerfTracker;
    void call(const PerfTracker::Samples& samples);

    CbFn cb_;
};
}  // namespace mle

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define MLE_PERF_ID(name_literal)                                                    \
    []() {                                                                           \
        static const auto _id = mle::PerfTracker::i().registerCounter(name_literal); \
        return _id;                                                                  \
    }()

/// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define MLE_PERF_SCOPE(name_literal)           \
    mle::PerfTracker::Scoped _mle_perf_scope { \
        MLE_PERF_ID(name_literal)              \
    }
