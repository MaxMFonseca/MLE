#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "mle/client/Layer.h"
#include "mle/client/SimulationClock.h"

namespace {
using Clock = mle::client::SimulationClock;
using namespace std::chrono_literals;

class TimingLayer final : public mle::client::Layer {
  public:
    void update(const mle::client::UpdateTiming& timing) override { update_timing = timing; }
    mle::ImageRef render(const mle::client::RenderTiming& timing) override {
        render_timing = timing;
        return nullptr;
    }
    void renderTo(mle::ImageRef, const mle::client::RenderTiming& timing) override { render_timing = timing; }

    mle::client::UpdateTiming update_timing{};
    mle::client::RenderTiming render_timing{};
};

TEST(LayerTiming, UpdateAndRenderContextsAreExplicit) {
    TimingLayer layer;
    const mle::client::UpdateTiming update_timing{.tick_id = 42, .fixed_dt = 1.0F / 60.0F, .scheduled_time = Clock::TimePoint{7s}};
    const mle::client::RenderTiming render_timing{.completed_tick_id = 42, .fixed_dt = 1.0F / 60.0F, .interpolation_alpha = 0.25F};

    layer.update(update_timing);
    layer.render(render_timing);
    layer.renderTo(nullptr, render_timing);

    EXPECT_EQ(layer.update_timing.tick_id, 42U);
    EXPECT_EQ(layer.update_timing.scheduled_time, Clock::TimePoint{7s});
    EXPECT_EQ(layer.render_timing.completed_tick_id, 42U);
    EXPECT_FLOAT_EQ(layer.render_timing.interpolation_alpha, 0.25F);
}

TEST(SimulationClock, CatchUpCapDropsStaleBacklog) {
    Clock clock;
    const auto start = Clock::TimePoint{1s};
    clock.start(start);

    std::vector<mle::client::UpdateTiming> updates;
    const auto now = start + (Clock::FIXED_PERIOD * 12);
    const auto run_batch = [&] { return clock.runDueUpdates(now, 5, [&](const auto& timing) { updates.push_back(timing); }); };

    EXPECT_EQ(run_batch(), 5U);
    EXPECT_EQ(run_batch(), 0U);
    ASSERT_EQ(updates.size(), 5U);
    for (std::uint64_t i = 0; i < updates.size(); ++i) {
        EXPECT_EQ(updates[i].tick_id, i + 1);
        EXPECT_FLOAT_EQ(updates[i].fixed_dt, 1.0F / 60.0F);
        EXPECT_EQ(updates[i].scheduled_time, start + (Clock::FIXED_PERIOD * static_cast<decltype(Clock::FIXED_PERIOD)::rep>(i + 1)));
    }

    const auto rebased_tick_time = now + Clock::FIXED_PERIOD;
    EXPECT_EQ(clock.nextScheduledTime(), rebased_tick_time);
    EXPECT_EQ(clock.runDueUpdates(rebased_tick_time, 5, [&](const auto& timing) { updates.push_back(timing); }), 1U);
    ASSERT_EQ(updates.size(), 6U);
    EXPECT_EQ(updates.back().tick_id, 6U);
    EXPECT_EQ(updates.back().scheduled_time, rebased_tick_time);
    EXPECT_FLOAT_EQ(clock.renderTiming(rebased_tick_time).interpolation_alpha, 0.0F);
}

TEST(SimulationClock, ScheduledTimesStayExactlyOnePeriodApart) {
    Clock clock;
    const auto start = Clock::TimePoint{2s};
    clock.start(start);

    std::vector<Clock::TimePoint> scheduled_times;
    clock.runDueUpdates(start + (Clock::FIXED_PERIOD * 4), 4, [&](const auto& timing) { scheduled_times.push_back(timing.scheduled_time); });

    ASSERT_EQ(scheduled_times.size(), 4U);
    EXPECT_EQ(scheduled_times[0], start + Clock::FIXED_PERIOD);
    EXPECT_EQ(scheduled_times[1] - scheduled_times[0], Clock::FIXED_PERIOD);
    EXPECT_EQ(scheduled_times[2] - scheduled_times[1], Clock::FIXED_PERIOD);
    EXPECT_EQ(scheduled_times[3] - scheduled_times[2], Clock::FIXED_PERIOD);
}

TEST(SimulationClock, PublishesCompletionOnlyAfterUpdateReturns) {
    Clock clock;
    const auto start = Clock::TimePoint{3s};
    clock.start(start);

    clock.runDueUpdates(start + Clock::FIXED_PERIOD, 1, [&](const auto& timing) {
        EXPECT_EQ(timing.tick_id, 1U);
        EXPECT_EQ(clock.renderTiming(timing.scheduled_time).completed_tick_id, 0U);
    });

    EXPECT_EQ(clock.renderTiming(start + Clock::FIXED_PERIOD).completed_tick_id, 1U);
}

TEST(SimulationClock, AlphaProgressesMonotonicallyBetweenTicks) {
    Clock clock;
    const auto start = Clock::TimePoint{4s};
    clock.start(start);
    clock.runDueUpdates(start + Clock::FIXED_PERIOD, 1, [](const auto&) {});

    const auto tick_time = start + Clock::FIXED_PERIOD;
    const auto alpha0 = clock.renderTiming(tick_time).interpolation_alpha;
    const auto alpha1 = clock.renderTiming(tick_time + (Clock::FIXED_PERIOD / 2)).interpolation_alpha;
    const auto alpha2 = clock.renderTiming(tick_time + Clock::FIXED_PERIOD).interpolation_alpha;

    EXPECT_FLOAT_EQ(alpha0, 0.0F);
    EXPECT_GT(alpha1, alpha0);
    EXPECT_LT(alpha1, alpha2);
    EXPECT_FLOAT_EQ(alpha2, 1.0F);
}

TEST(SimulationClock, AlphaClampsAfterStalls) {
    Clock clock;
    const auto start = Clock::TimePoint{5s};
    clock.start(start);
    clock.runDueUpdates(start + Clock::FIXED_PERIOD, 1, [](const auto&) {});

    EXPECT_FLOAT_EQ(clock.renderTiming(start).interpolation_alpha, 0.0F);
    EXPECT_FLOAT_EQ(clock.renderTiming(start + 10s).interpolation_alpha, 1.0F);
}

TEST(SimulationClock, ConcurrentRenderReadsObservePublishedTicksSafely) {
    Clock clock;
    const auto start = Clock::TimePoint{6s};
    clock.start(start);

    constexpr std::uint64_t tick_count = 128;
    std::atomic<bool> invalid_read = false;
    std::atomic<std::uint64_t> observed_tick = 0;
    std::vector<std::uint64_t> snapshots(tick_count + 1, 0);

    std::thread reader([&] {
        std::uint64_t previous_tick = 0;
        while (previous_tick < tick_count) {
            const auto timing = clock.renderTiming(start + (Clock::FIXED_PERIOD * tick_count));
            if (timing.completed_tick_id < previous_tick || timing.completed_tick_id > tick_count || timing.fixed_dt != Clock::FIXED_DT ||
                timing.interpolation_alpha < 0.0F || timing.interpolation_alpha > 1.0F ||
                (timing.completed_tick_id > 0 && snapshots[timing.completed_tick_id] != timing.completed_tick_id)) {
                invalid_read.store(true, std::memory_order_relaxed);
            }
            if (timing.completed_tick_id > previous_tick) {
                previous_tick = timing.completed_tick_id;
                observed_tick.store(previous_tick, std::memory_order_release);
            }
            std::this_thread::yield();
        }
    });

    for (std::uint64_t tick_id = 1; tick_id <= tick_count; ++tick_id) {
        clock.runDueUpdates(start + (Clock::FIXED_PERIOD * static_cast<decltype(Clock::FIXED_PERIOD)::rep>(tick_id)), 1,
                            [&](const auto& timing) { snapshots[timing.tick_id] = timing.tick_id; });
        while (observed_tick.load(std::memory_order_acquire) < tick_id) {
            std::this_thread::yield();
        }
    }
    reader.join();

    EXPECT_FALSE(invalid_read.load(std::memory_order_relaxed));
    EXPECT_EQ(observed_tick.load(std::memory_order_relaxed), tick_count);
    EXPECT_EQ(clock.renderTiming(start + (Clock::FIXED_PERIOD * tick_count)).completed_tick_id, tick_count);
}
}  // namespace
