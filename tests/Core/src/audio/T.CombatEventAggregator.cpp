#include <gtest/gtest.h>

#include <array>

#include "audio/CombatEventAggregator.h"

namespace mle::audio {
namespace {
struct SimulationTotals {
    u32 raw{};
    u32 aggregated{};
    u32 submitted{};
    u32 dropped{};
};

SimulationTotals simulate(u32 events_per_second, u32 frames_per_second) {
    CombatEventAggregator aggregator;
    SimulationTotals totals{};
    u32 event_remainder = 0;

    for (u32 frame = 0; frame < frames_per_second * 10; ++frame) {
        event_remainder += events_per_second;
        const u32 accepted_events = event_remainder / frames_per_second;
        event_remainder %= frames_per_second;

        const CombatFrameResult result = aggregator.advance(1.0F / static_cast<f32>(frames_per_second), accepted_events);
        EXPECT_LE(result.submitted, result.cues.size());
        EXPECT_EQ(result.raw, result.aggregated + result.dropped);
        EXPECT_LE(aggregator.pendingEvents(), CombatEventAggregator::MAX_PENDING_EVENTS);

        u32 represented = 0;
        u32 previous_priority = CombatCue::MAX_PRIORITY;
        for (u32 i = 0; i < result.submitted; ++i) {
            EXPECT_GT(result.cues.at(i).event_count, 0U);
            EXPECT_LE(result.cues.at(i).priority, previous_priority);
            previous_priority = result.cues.at(i).priority;
            represented += result.cues.at(i).event_count;
        }
        EXPECT_EQ(represented, result.aggregated);

        totals.raw += result.raw;
        totals.aggregated += result.aggregated;
        totals.submitted += result.submitted;
        totals.dropped += result.dropped;
    }

    EXPECT_EQ(event_remainder, 0U);
    return totals;
}
}  // namespace

TEST(CombatEventAggregator, SustainedRatesAreFrameRateIndependent) {
    constexpr std::array RATES{100U, 250U, 500U};
    constexpr std::array FRAME_RATES{30U, 60U, 144U};

    for (const u32 rate : RATES) {
        const SimulationTotals expected = simulate(rate, FRAME_RATES.front());
        EXPECT_EQ(expected.raw, rate * 10U);
        EXPECT_EQ(expected.raw, expected.aggregated + expected.dropped);

        for (const u32 frame_rate : FRAME_RATES) {
            const SimulationTotals actual = simulate(rate, frame_rate);
            EXPECT_EQ(actual.raw, expected.raw);
            EXPECT_EQ(actual.aggregated, expected.aggregated);
            EXPECT_EQ(actual.dropped, expected.dropped);
        }
    }
}

TEST(CombatEventAggregator, CoalescesBurstIntoFourPriorityOrderedCues) {
    CombatEventAggregator aggregator;

    const CombatFrameResult result = aggregator.advance(1.0F / 60.0F, 100U);

    EXPECT_EQ(result.raw, 100U);
    EXPECT_EQ(result.aggregated, 100U);
    EXPECT_EQ(result.submitted, 4U);
    EXPECT_EQ(result.dropped, 0U);
    EXPECT_EQ(aggregator.pendingEvents(), 0U);
}

TEST(CombatEventAggregator, EmptyFrameProducesNoCue) {
    CombatEventAggregator aggregator;
    const CombatFrameResult result = aggregator.advance(1.0F / 60.0F, 0U);

    EXPECT_EQ(result.raw, 0U);
    EXPECT_EQ(result.aggregated, 0U);
    EXPECT_EQ(result.submitted, 0U);
    EXPECT_EQ(result.dropped, 0U);
    EXPECT_EQ(aggregator.pendingEvents(), 0U);
}
}  // namespace mle::audio
