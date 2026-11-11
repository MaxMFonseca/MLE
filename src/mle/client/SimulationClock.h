#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <utility>

namespace mle::client {
struct UpdateTiming {
    std::uint64_t tick_id{};
    float fixed_dt{};
    std::chrono::steady_clock::time_point scheduled_time{};
};

struct RenderTiming {
    std::uint64_t completed_tick_id{};
    float fixed_dt{};
    float interpolation_alpha{};
};

// The simulation thread owns start/runDueUpdates/nextScheduledTime. Once started,
// renderTiming may be read concurrently until the render thread is stopped.
class SimulationClock {
  public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    struct CompletedTick {
        std::uint64_t tick_id{};
        Clock::duration::rep scheduled_time_count{};
    };

    static constexpr auto FIXED_PERIOD = std::chrono::nanoseconds{16'666'667};
    static constexpr float FIXED_DT = 1.0F / 60.0F;

    void start(TimePoint start_time) {
        next_scheduled_time_ = start_time + FIXED_PERIOD;
        next_tick_id_ = 1;
        completed_tick_.store({}, std::memory_order_relaxed);
        started_.store(true, std::memory_order_release);
    }

    template <typename UpdateFn>
    std::uint32_t runDueUpdates(TimePoint now, std::uint32_t max_updates, UpdateFn&& update) {
        if (!started_.load(std::memory_order_acquire)) {
            return 0;
        }

        std::uint32_t update_count = 0;
        while (next_scheduled_time_ <= now && update_count < max_updates) {
            const UpdateTiming timing{
                .tick_id = next_tick_id_,
                .fixed_dt = FIXED_DT,
                .scheduled_time = next_scheduled_time_,
            };

            std::invoke(update, timing);
            completed_tick_.store({.tick_id = next_tick_id_, .scheduled_time_count = next_scheduled_time_.time_since_epoch().count()},
                                  std::memory_order_release);

            ++next_tick_id_;
            next_scheduled_time_ += FIXED_PERIOD;
            ++update_count;
        }

        if (update_count == max_updates && next_scheduled_time_ <= now) {
            next_scheduled_time_ = now + FIXED_PERIOD;
        }
        return update_count;
    }

    [[nodiscard]] RenderTiming renderTiming(TimePoint now) const {
        if (!started_.load(std::memory_order_acquire)) {
            return {.completed_tick_id = 0, .fixed_dt = FIXED_DT, .interpolation_alpha = 0.0F};
        }

        const auto completed_tick = completed_tick_.load(std::memory_order_acquire);
        if (completed_tick.tick_id == 0) {
            return {.completed_tick_id = 0, .fixed_dt = FIXED_DT, .interpolation_alpha = 0.0F};
        }

        const auto completed_scheduled_time = TimePoint{Clock::duration{completed_tick.scheduled_time_count}};
        const auto elapsed = std::chrono::duration<float>(now - completed_scheduled_time).count();
        const auto fixed_period = std::chrono::duration<float>(FIXED_PERIOD).count();
        return {
            .completed_tick_id = completed_tick.tick_id,
            .fixed_dt = FIXED_DT,
            .interpolation_alpha = std::clamp(elapsed / fixed_period, 0.0F, 1.0F),
        };
    }

    [[nodiscard]] TimePoint nextScheduledTime() const { return next_scheduled_time_; }

  private:
    TimePoint next_scheduled_time_{};
    std::uint64_t next_tick_id_ = 1;
    std::atomic<CompletedTick> completed_tick_{};
    std::atomic<bool> started_ = false;
};
}  // namespace mle::client
