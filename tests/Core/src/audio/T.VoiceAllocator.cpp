#include <gtest/gtest.h>

#include <array>

#include "mle/audio/Types.h"
#include "mle/audio/VoiceAllocator.h"

namespace mle::audio {
namespace {
std::array<BusVoicePolicy, BUS_COUNT> policies() {
    return {};
}
}  // namespace

TEST(VoiceAllocator, ChoosesLowestFreeIndex) {
    std::array voices{VoiceMetadata{.priority = 0, .bus = 7}, VoiceMetadata{.priority = 0}, VoiceMetadata{.priority = 3}};
    EXPECT_EQ(selectVoice(voices, policies(), 0, 1).index, 0U);
}

TEST(VoiceAllocator, UnlimitedBusStealsGloballyLowestPriority) {
    std::array voices{VoiceMetadata{.priority = 5, .bus = 1}, VoiceMetadata{.priority = 2, .bus = 2}, VoiceMetadata{.priority = 4, .bus = 3}};
    EXPECT_EQ(selectVoice(voices, policies(), 0, 3).index, 1U);
}

TEST(VoiceAllocator, AtCapReplacesOnlyLowerSameBus) {
    std::array voices{VoiceMetadata{.priority = 2, .bus = 3}, VoiceMetadata{.priority = 5, .bus = 3}, VoiceMetadata{.priority = 1, .bus = 4}};
    auto bus_policies = policies();
    bus_policies[3].max_voices = 2;
    EXPECT_EQ(selectVoice(voices, bus_policies, 3, 3).index, 0U);
    EXPECT_EQ(selectVoice(voices, bus_policies, 3, 2).reason, VoiceRejectReason::INSUFFICIENT_PRIORITY);
}

TEST(VoiceAllocator, EqualPriorityCannotSteal) {
    std::array voices{VoiceMetadata{.priority = 2, .bus = 1}};
    EXPECT_EQ(selectVoice(voices, policies(), 0, 2).reason, VoiceRejectReason::INSUFFICIENT_PRIORITY);
}

TEST(VoiceAllocator, ProtectedBusCannotBeStolenAcrossBuses) {
    std::array voices{VoiceMetadata{.priority = 1, .bus = 2}, VoiceMetadata{.priority = 4, .bus = 3}};
    auto bus_policies = policies();
    bus_policies[2].protected_from_other_buses = true;
    EXPECT_EQ(selectVoice(voices, bus_policies, 1, 5).index, 1U);
}

TEST(VoiceAllocator, ProtectedBusCanReplaceItsOwnVoice) {
    std::array voices{VoiceMetadata{.priority = 1, .bus = 2}};
    auto bus_policies = policies();
    bus_policies[2].protected_from_other_buses = true;
    EXPECT_EQ(selectVoice(voices, bus_policies, 2, 2).index, 0U);
}

TEST(VoiceAllocator, ProtectedRequesterCanStealUnprotectedBus) {
    std::array voices{VoiceMetadata{.priority = 1, .bus = 1}};
    auto bus_policies = policies();
    bus_policies[2].protected_from_other_buses = true;
    EXPECT_EQ(selectVoice(voices, bus_policies, 2, 2).index, 0U);
}

TEST(VoiceAllocator, RejectsWhenNoVictimIsEligible) {
    std::array voices{VoiceMetadata{.priority = 1, .bus = 1}, VoiceMetadata{.priority = 2, .bus = 2}};
    auto bus_policies = policies();
    bus_policies[1].protected_from_other_buses = true;
    bus_policies[2].protected_from_other_buses = true;
    EXPECT_EQ(selectVoice(voices, bus_policies, 0, 10).reason, VoiceRejectReason::NO_ELIGIBLE_VICTIM);
}

TEST(VoiceAllocator, BreaksVictimTiesByLowestIndex) {
    std::array voices{VoiceMetadata{.priority = 1, .bus = 1}, VoiceMetadata{.priority = 1, .bus = 2}};
    EXPECT_EQ(selectVoice(voices, policies(), 0, 2).index, 0U);
}

TEST(VoiceAllocator, RejectsInvalidBus) {
    std::array voices{VoiceMetadata{}};
    EXPECT_EQ(selectVoice(voices, policies(), BUS_COUNT, 1).reason, VoiceRejectReason::INVALID_BUS);
}

TEST(VoiceAllocator, RejectsZeroPriority) {
    std::array voices{VoiceMetadata{}};
    EXPECT_EQ(selectVoice(voices, policies(), 0, 0).reason, VoiceRejectReason::ZERO_PRIORITY);
}

TEST(VoiceAllocator, ZeroCapMeansUnlimited) {
    std::array voices{VoiceMetadata{.priority = 1, .bus = 1}};
    auto bus_policies = policies();
    bus_policies[0].max_voices = 0;
    EXPECT_TRUE(selectVoice(voices, bus_policies, 0, 2).accepted());
}

TEST(VoiceAllocator, EmptyPoolHasNoEligibleVictim) {
    std::array<VoiceMetadata, 0> voices{};
    EXPECT_EQ(selectVoice(voices, policies(), 0, 1).reason, VoiceRejectReason::NO_ELIGIBLE_VICTIM);
}

TEST(VoiceAllocator, ResetRestoresFreeDefaults) {
    VoiceMetadata voice{.priority = 9, .bus = 7, .volume = 0.25F};
    resetVoice(voice);
    EXPECT_EQ(voice.priority, 0U);
    EXPECT_EQ(voice.bus, 0U);
    EXPECT_FLOAT_EQ(voice.volume, 1.0F);
}

TEST(VoiceAllocator, BusVoicePolicyCommandHasSafeDefaultsAndFitsCommandVariant) {
    audio::cmd::SetBusVoicePolicy defaults{};
    EXPECT_EQ(defaults.bus, 0U);
    EXPECT_EQ(defaults.max_voices, 0U);
    EXPECT_FALSE(defaults.protected_from_other_buses);

    audio::cmd::SetBusVoicePolicy cmd{.bus = 3, .max_voices = 16, .protected_from_other_buses = false};
    audio::Cmd variant = cmd;
    EXPECT_TRUE(std::holds_alternative<audio::cmd::SetBusVoicePolicy>(variant));
}
}  // namespace mle::audio
