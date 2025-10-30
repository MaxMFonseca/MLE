#include "PerfTracker.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <utility>

#include "mle/core/Assert.h"

namespace mle {
PerfTracker::CounterId PerfTracker::registerCounter(const char* name) {
    if constexpr (!ENABLED) {
        return 0;
    }

    std::scoped_lock<std::mutex> lock(names_mtx_);
    auto it = name_to_id_.find(name);
    if (it != name_to_id_.end()) {
        return it->second;
    }

    const auto id = as<CounterId>(id_to_name_.size());
    id_to_name_.emplace_back(name);
    name_to_id_.emplace(id_to_name_.back(), id);
    return id;
}

void PerfTracker::record(CounterId id, std::chrono::nanoseconds ns) {
    if constexpr (!ENABLED) {
        return;
    }

    if (id >= entries_.size()) {
        std::scoped_lock<std::mutex> lock(entries_resize_mtx_);
        entries_.resize(id + 1);
    }
    auto& e = entries_[id];
    e.ns.fetch_add(ns.count(), std::memory_order_relaxed);
    e.calls.fetch_add(1, std::memory_order_relaxed);
}

// NOLINTNEXTLINE(performance-unnecessary-value-param) no need
void PerfTracker::update(std::stop_token st) {
    if constexpr (!ENABLED) {
        return;
    }

    using namespace std::chrono_literals;
    const auto interval = 1s;
    auto next = std::chrono::steady_clock::now() + interval;
    while (!st.stop_requested()) {
        std::vector<Entry> acc;

        {
            std::scoped_lock<std::mutex> lock(entries_resize_mtx_);

            acc.resize(id_to_name_.size());

            for (usize id = 0; id < entries_.size(); ++id) {
                if (id >= acc.size()) {
                    acc.resize(id + 1);
                }
                auto& be = entries_[id];
                if (be.calls.load(std::memory_order_relaxed) == 0) {
                    continue;
                }
                auto& ae = acc[id];
                ae.ns += be.ns.exchange(0, std::memory_order_relaxed);
                ae.calls += be.calls.exchange(0, std::memory_order_relaxed);
            }
        }

        if (!acc.empty()) {
            Samples samples;
            samples.resize(acc.size());

            {
                std::scoped_lock<std::mutex> lock(names_mtx_);
                for (u64 id = 0; id < acc.size(); ++id) {
                    auto& entry = acc[id];
                    if (entry.calls == 0) {
                        continue;
                    }
                    const f64 us_per_call = (as<f64>(entry.ns) / as<f64>(entry.calls)) / 1000.0;

                    if (lifetime_acc_.size() <= id) {
                        lifetime_acc_.resize(id + 1);
                    }
                    lifetime_acc_[id].ns += entry.ns;
                    lifetime_acc_[id].calls += entry.calls;

                    samples[id] = Sample{.name = id_to_name_.at(id), .last_sec_ns = entry.ns, .per_call_us = us_per_call, .calls_last_sec = entry.calls};
                }
            }

            {
                std::vector<PerfNewSamplesListenerRef> listeners_copy;
                {
                    std::scoped_lock lock(new_samples_listeners_mtx_);
                    listeners_copy.assign(new_samples_listeners_.begin(), new_samples_listeners_.end());
                }
                for (auto* listener : listeners_copy) {
                    if (listener) {
                        listener->call(samples);
                    }
                }

                static u32 counter = 0;
                MLE_I("Perf {} (avg µs/call over last second):", counter);
                counter++;
                for (const auto& s : samples) {
                    MLE_I("  {}: {:.3f} µs ({} calls)", s.name, s.per_call_us, s.calls_last_sec);
                }
            }
        }

        std::this_thread::sleep_until(next);
        next += interval;
    }
}

void PerfTracker::init() {
    if constexpr (!ENABLED) {
        MLE_I("PerfTracker disabled");
        return;
    }

    MLE_D("PerfTracker init");
    first_time_ = std::chrono::steady_clock::now();
    update_thread_ = std::jthread([this](std::stop_token st) { update(std::move(st)); });
}

void PerfTracker::shutdown() {
    if constexpr (!ENABLED) {
        return;
    }

    if (update_thread_.joinable()) {
        update_thread_.request_stop();
        update_thread_.join();
    }

    MLE_I("PerfTracker shutdown:");
    MLE_I("  Uptime: {:.1f} seconds", std::chrono::duration<f64>(std::chrono::steady_clock::now() - first_time_).count());
    MLE_I("  Lifetime averages:");
    for (u64 id = 0; id < lifetime_acc_.size(); ++id) {
        auto& entry = lifetime_acc_[id];
        if (entry.calls == 0) {
            continue;
        }
        const f64 us_per_call = (as<f64>(entry.ns) / as<f64>(entry.calls)) / 1000.0;
        const char* name = idToName(as<CounterId>(id));
        MLE_I("    {}: {:.3f} µs ({} calls)", name ? name : "<unnamed>", us_per_call, entry.calls);
    }
}

const char* PerfTracker::idToName(CounterId id) {
    std::scoped_lock<std::mutex> lock(names_mtx_);
    if (id < id_to_name_.size()) {
        return id_to_name_[id].c_str();
    }
    MLE_UNREACHABLE_LOG("Tried to get name for invalid CounterId {}", id);
}

void PerfTracker::addNewSamplesListener(PerfNewSamplesListener* l) {
    if constexpr (!ENABLED) {
        return;
    }

    std::scoped_lock<std::mutex> lock(new_samples_listeners_mtx_);
    new_samples_listeners_.insert(l);
}

void PerfTracker::removeNewSamplesListener(PerfNewSamplesListener* l) {
    if constexpr (!ENABLED) {
        return;
    }

    std::scoped_lock<std::mutex> lock(new_samples_listeners_mtx_);
    new_samples_listeners_.erase(l);
}

void PerfNewSamplesListener::call(const PerfTracker::Samples& samples) {
    MLE_ASSERT(cb_);
    cb_(samples);
};
}  // namespace mle
