#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <memory>
#include <unordered_map>

#include "mle/audio/StreamState.h"
#include "mle/audio/Types.h"
#include "mle/audio/Utils.h"
#include "mle/audio/VoiceAllocator.h"

namespace mle::audio {
using namespace std::chrono_literals;

TEST(StreamState, ComputesZeroDurationToEnd) {
    EXPECT_EQ(computeSampleWindow(48'000, 2, 192'000, 500, 0).value(), (SampleWindow{.first_sample = 48'000, .last_sample = 192'000}));
}

TEST(StreamState, ComputesNonzeroOffsetAndDurationInInterleavedSamples) {
    EXPECT_EQ(computeSampleWindow(48'000, 2, 192'000, 500, 250).value(), (SampleWindow{.first_sample = 48'000, .last_sample = 72'000}));
}

TEST(StreamState, ClampsOverlongDurationToEnd) {
    EXPECT_EQ(computeSampleWindow(48'000, 2, 192'000, 1500, 1000).value(), (SampleWindow{.first_sample = 144'000, .last_sample = 192'000}));
}

TEST(StreamState, RejectsPositiveDurationRoundingToZeroSamples) {
    EXPECT_EQ(computeSampleWindow(1, 1, 10, 0, 1).error(), Result::OUT_OR_RANGE);
}

TEST(StreamState, RejectsOffsetAtOrBeyondEnd) {
    EXPECT_EQ(computeSampleWindow(1000, 1, 1000, 1000, 0).error(), Result::OUT_OR_RANGE);
    EXPECT_EQ(computeSampleWindow(1000, 1, 1000, 1001, 0).error(), Result::OUT_OR_RANGE);
}

TEST(StreamState, RejectsInvalidChannelCount) {
    EXPECT_EQ(computeSampleWindow(48'000, 0, 192'000, 0, 0).error(), Result::INVALID_ARGUMENT);
}

TEST(StreamState, PreservesMonoAndStereoInterleavingUnits) {
    EXPECT_EQ(computeSampleWindow(1000, 1, 5000, 250, 500).value(), (SampleWindow{.first_sample = 250, .last_sample = 750}));
    EXPECT_EQ(computeSampleWindow(1000, 2, 5000, 250, 500).value(), (SampleWindow{.first_sample = 500, .last_sample = 1500}));
}

TEST(StreamState, FillsLoopingDestinationAcrossRepeatedShortWindowWraps) {
    const std::array<f32, 10> samples{0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F};
    std::array<f32, 8> destination{};

    const usize next_sample = fillLoopingSampleWindow(samples, SampleWindow{.first_sample = 8, .last_sample = 10}, 9, destination);

    EXPECT_EQ(destination, (std::array<f32, 8>{9.0F, 8.0F, 9.0F, 8.0F, 9.0F, 8.0F, 9.0F, 8.0F}));
    EXPECT_EQ(next_sample, 9);

    destination.fill(0.0F);
    EXPECT_EQ(fillLoopingSampleWindow(samples, SampleWindow{.first_sample = 9, .last_sample = 10}, 9, destination), 9);
    EXPECT_EQ(destination, (std::array<f32, 8>{9.0F, 9.0F, 9.0F, 9.0F, 9.0F, 9.0F, 9.0F, 9.0F}));
}

TEST(StreamState, NormalizesLoopingCursorAtExactWindowMultiples) {
    constexpr SampleWindow WINDOW{.first_sample = 8, .last_sample = 16};
    EXPECT_EQ(advanceLoopingSampleCursor(WINDOW, 8, 8), 8);
    EXPECT_EQ(advanceLoopingSampleCursor(WINDOW, 8, 24), 8);
    EXPECT_EQ(advanceLoopingSampleCursor(WINDOW, 12, 12), 8);
}

TEST(StreamState, ActiveSlotRetainsDecodedDataWhenCacheEntryIsReplaced) {
    constexpr entt::id_type SOUND_ID = 42;
    std::unordered_map<entt::id_type, std::shared_ptr<const WavData>> cache;
    std::shared_ptr<const WavData> slot;
    auto original = std::make_shared<const WavData>(WavData{.samples = {1.0F}, .channels = 1, .sample_rate = 48'000});
    std::weak_ptr<const WavData> original_weak = original;

    cache[SOUND_ID] = original;
    slot = cache.at(SOUND_ID);
    cache[SOUND_ID] = std::make_shared<const WavData>(WavData{.samples = {2.0F}, .channels = 1, .sample_rate = 48'000});
    original.reset();

    ASSERT_FALSE(original_weak.expired());
    ASSERT_NE(slot, nullptr);
    EXPECT_FLOAT_EQ(slot->samples.front(), 1.0F);
    EXPECT_FLOAT_EQ(cache.at(SOUND_ID)->samples.front(), 2.0F);
}

TEST(StreamState, RejectsInvalidBusDuringStartPreflight) {
    EXPECT_EQ(validateStreamStart(48'000, 2, 192'000, BUS_COUNT, 1.0F, 1.0F, 0, 0, 8192).error(), Result::OUT_OR_RANGE);
}

TEST(StreamState, FailedStartPreflightLeavesOwnedSlotUntouched) {
    const auto occupied = std::make_shared<const WavData>(WavData{.samples = {1.0F}, .channels = 1, .sample_rate = 48'000});
    std::shared_ptr<const WavData> slot = occupied;
    const auto replacement = std::make_shared<const WavData>(WavData{.samples = std::vector<f32>(8192), .channels = 1, .sample_rate = 48'000});

    const auto start = validateStreamStart(replacement->sample_rate, as<u16>(replacement->channels), replacement->samples.size(), 0, 1.0F, 1.0F, 1000, 0, 8192);
    if (start) {
        slot = replacement;
    }

    EXPECT_FALSE(start);
    EXPECT_EQ(slot, occupied);
}

TEST(StreamState, CompletesZeroDurationImmediately) {
    auto ramp = beginRamp(0.2F, 1.0F, 0ms, RampCompletion::NONE);
    const auto result = advanceRamp(ramp, 0ms);
    EXPECT_FLOAT_EQ(result.volume, 1.0F);
    EXPECT_FALSE(ramp.active);
}

TEST(StreamState, ReportsZeroDurationStopExactlyOnce) {
    auto ramp = beginRamp(1.0F, 0.0F, 0ms, RampCompletion::STOP);
    EXPECT_EQ(advanceRamp(ramp, 0ms).completion, RampCompletion::STOP);
    EXPECT_EQ(advanceRamp(ramp, 0ms).completion, RampCompletion::NONE);
}

TEST(StreamState, AdvancesThroughMidpointAndExactCompletion) {
    auto ramp = beginRamp(0.2F, 1.0F, 1000ms, RampCompletion::NONE);
    EXPECT_FLOAT_EQ(advanceRamp(ramp, 500ms).volume, 0.6F);
    EXPECT_FLOAT_EQ(advanceRamp(ramp, 500ms).volume, 1.0F);
    EXPECT_FALSE(ramp.active);
}

TEST(StreamState, ClampsOvershootExactlyToTarget) {
    auto ramp = beginRamp(1.0F, 0.3F, 1000ms, RampCompletion::NONE);
    EXPECT_FLOAT_EQ(advanceRamp(ramp, 1500ms).volume, 0.3F);
    EXPECT_FLOAT_EQ(ramp.current, 0.3F);
    EXPECT_FALSE(ramp.active);
}

TEST(StreamState, ClampsMaximumDeltaWithoutSignedOverflow) {
    auto ramp = beginRamp(0.0F, 1.0F, 1000ms, RampCompletion::NONE);
    advanceRamp(ramp, 1ms);
    EXPECT_FLOAT_EQ(advanceRamp(ramp, std::chrono::milliseconds::max()).volume, 1.0F);
    EXPECT_FALSE(ramp.active);
}

TEST(StreamState, ReplacementStartsFromCurrentValue) {
    auto ramp = beginRamp(0.0F, 1.0F, 1000ms, RampCompletion::NONE);
    advanceRamp(ramp, 250ms);
    ramp = beginRamp(ramp.current, 0.75F, 500ms, RampCompletion::NONE);
    EXPECT_FLOAT_EQ(ramp.start, 0.25F);
    EXPECT_FLOAT_EQ(advanceRamp(ramp, 250ms).volume, 0.5F);
}

TEST(StreamState, ZeroDeltaPausesAndLaterDeltaResumes) {
    auto ramp = beginRamp(0.0F, 1.0F, 1000ms, RampCompletion::NONE);
    EXPECT_FLOAT_EQ(advanceRamp(ramp, 250ms).volume, 0.25F);
    EXPECT_FLOAT_EQ(advanceRamp(ramp, 0ms).volume, 0.25F);
    EXPECT_EQ(ramp.elapsed, 250ms);
    EXPECT_FLOAT_EQ(advanceRamp(ramp, 250ms).volume, 0.5F);
}

TEST(StreamState, ReportsStopOnlyOnCompletion) {
    auto ramp = beginRamp(1.0F, 0.0F, 100ms, RampCompletion::STOP);
    EXPECT_EQ(advanceRamp(ramp, 50ms).completion, RampCompletion::NONE);
    EXPECT_EQ(advanceRamp(ramp, 50ms).completion, RampCompletion::STOP);
}

TEST(StreamState, ConstructsLiveStreamParameterCommands) {
    cmd::SetStreamParams params{.id = 5, .volume = 0.4F, .pitch = std::nullopt, .fade_ms = 250};
    EXPECT_TRUE(params.volume.has_value());
    EXPECT_FALSE(params.pitch.has_value());

    Cmd variant = params;
    EXPECT_TRUE(std::holds_alternative<cmd::SetStreamParams>(variant));

    const cmd::StopStream stop{.id = 5, .fade_out_ms = 500};
    EXPECT_EQ(stop.fade_out_ms, 500U);
}
}  // namespace mle::audio
