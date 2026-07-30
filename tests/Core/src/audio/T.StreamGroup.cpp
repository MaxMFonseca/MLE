#include <gtest/gtest.h>

#include <algorithm>
#include <array>

#include "mle/audio/StreamGroup.h"

namespace mle::audio {
namespace {
struct PlayvRecorder {
    u32 call_count{};
    ALsizei played_count{};
    std::array<ALuint, STREAM_SLOT_COUNT> played_sources{};
};

PlayvRecorder& playvRecorder() {
    static PlayvRecorder recorder;
    return recorder;
}

bool recordPlayv(ALsizei count, const ALuint* sources) {
    auto& recorder = playvRecorder();
    ++recorder.call_count;
    recorder.played_count = count;
    std::copy_n(sources, static_cast<usize>(count), recorder.played_sources.begin());
    return true;
}

bool rejectPlayv(ALsizei count, const ALuint* sources) {
    auto& recorder = playvRecorder();
    ++recorder.call_count;
    recorder.played_count = count;
    std::copy_n(sources, static_cast<usize>(count), recorder.played_sources.begin());
    return false;
}

cmd::StartStreamGroup groupWithCount(u8 count) {
    cmd::StartStreamGroup group{};
    group.count = count;
    const auto initialized_count = std::min<u8>(count, static_cast<u8>(STREAM_SLOT_COUNT));
    for (u8 i = 0; i < initialized_count; ++i) {
        group.streams.at(i).id = i;
    }
    return group;
}
}  // namespace

TEST(StreamGroup, RejectsZeroAndOversizedCounts) {
    EXPECT_EQ(validateStreamGroup(groupWithCount(0)), StreamGroupRejectReason::EMPTY);
    EXPECT_EQ(validateStreamGroup(groupWithCount(STREAM_SLOT_COUNT + 1)), StreamGroupRejectReason::TOO_MANY);
}

TEST(StreamGroup, AcceptsEveryCountFromOneThroughCapacity) {
    for (u8 count = 1; count <= STREAM_SLOT_COUNT; ++count) {
        EXPECT_EQ(validateStreamGroup(groupWithCount(count)), StreamGroupRejectReason::NONE);
    }
}

TEST(StreamGroup, RejectsInvalidAndDuplicateSlots) {
    auto invalid = groupWithCount(1);
    invalid.streams[0].id = STREAM_SLOT_COUNT;
    EXPECT_EQ(validateStreamGroup(invalid), StreamGroupRejectReason::INVALID_SLOT);

    auto duplicate = groupWithCount(2);
    duplicate.streams[1].id = duplicate.streams[0].id;
    EXPECT_EQ(validateStreamGroup(duplicate), StreamGroupRejectReason::DUPLICATE_SLOT);
}

TEST(StreamGroup, UsesEightFixedEntries) {
    const cmd::StartStreamGroup group{};
    EXPECT_EQ(STREAM_SLOT_COUNT, 8U);
    EXPECT_EQ(group.streams.size(), STREAM_SLOT_COUNT);
}

TEST(StreamGroup, InvokesPlayvExactlyOnceInSourceOrder) {
    auto& recorder = playvRecorder();
    recorder = {};
    constexpr std::array<ALuint, 4> SOURCES{41, 7, 99, 13};

    EXPECT_TRUE(invokeSynchronizedPlay(SOURCES, recordPlayv));

    EXPECT_EQ(recorder.call_count, 1U);
    EXPECT_EQ(recorder.played_count, 4);
    EXPECT_EQ(recorder.played_sources[0], 41U);
    EXPECT_EQ(recorder.played_sources[1], 7U);
    EXPECT_EQ(recorder.played_sources[2], 99U);
    EXPECT_EQ(recorder.played_sources[3], 13U);
}

TEST(StreamGroup, ForwardsPlayvRejectionExactlyOnceInSourceOrder) {
    auto& recorder = playvRecorder();
    recorder = {};
    constexpr std::array<ALuint, 4> SOURCES{41, 7, 99, 13};

    EXPECT_FALSE(invokeSynchronizedPlay(SOURCES, rejectPlayv));

    EXPECT_EQ(recorder.call_count, 1U);
    EXPECT_EQ(recorder.played_count, 4);
    EXPECT_EQ(recorder.played_sources[0], 41U);
    EXPECT_EQ(recorder.played_sources[1], 7U);
    EXPECT_EQ(recorder.played_sources[2], 99U);
    EXPECT_EQ(recorder.played_sources[3], 13U);
}

TEST(StreamGroup, DescribesEveryRejectReason) {
    EXPECT_EQ(streamGroupRejectReasonName(StreamGroupRejectReason::NONE), "none");
    EXPECT_EQ(streamGroupRejectReasonName(StreamGroupRejectReason::EMPTY), "empty");
    EXPECT_EQ(streamGroupRejectReasonName(StreamGroupRejectReason::TOO_MANY), "too many");
    EXPECT_EQ(streamGroupRejectReasonName(StreamGroupRejectReason::INVALID_SLOT), "invalid slot");
    EXPECT_EQ(streamGroupRejectReasonName(StreamGroupRejectReason::DUPLICATE_SLOT), "duplicate slot");
}

TEST(StreamGroup, CommandFitsAudioVariant) {
    cmd::StartStreamGroup group = groupWithCount(3);
    Cmd command = group;
    EXPECT_TRUE(std::holds_alternative<cmd::StartStreamGroup>(command));
}
}  // namespace mle::audio
