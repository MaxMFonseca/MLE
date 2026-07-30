#include <gtest/gtest.h>

#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "mle/audio/AudioThreadStartup.h"
#include "mle/audio/CommandMailbox.h"
#include "mle/audio/Types.h"
#include "mle/client/ShutdownSequence.h"

namespace mle {
namespace {
TEST(AudioShutdown, ConsumesLayerStopStreamBeforeAudioTeardown) {
    audio::CommandMailbox mailbox{8};
    mailbox.open();
    std::vector<std::string> events;
    std::vector<u8> stopped_streams;
    const auto client_thread = std::this_thread::get_id();
    std::thread::id command_thread;

    client::shutdownLayersBeforeAudio(
        [&] {
            events.emplace_back("game layer shutdown");
            EXPECT_EQ(mailbox.tryPush(audio::cmd::StopStream{.id = 5}), audio::CommandSubmitResult::ACCEPTED);
            EXPECT_EQ(mailbox.tryPush(audio::cmd::StopStream{.id = 7}), audio::CommandSubmitResult::ACCEPTED);
        },
        [&] { events.emplace_back("debug layers shutdown"); },
        [&] {
            mailbox.close();
            std::jthread audio_thread([&] {
                audio::drainBeforeTeardown(
                    mailbox,
                    [&](const audio::Cmd& cmd) {
                        command_thread = std::this_thread::get_id();
                        ASSERT_TRUE(std::holds_alternative<audio::cmd::StopStream>(cmd));
                        stopped_streams.push_back(std::get<audio::cmd::StopStream>(cmd).id);
                        events.emplace_back("stop stream consumed");
                    },
                    [&] { events.emplace_back("audio teardown"); });
            });
            audio_thread.join();
        });

    EXPECT_NE(command_thread, client_thread);
    EXPECT_EQ(events, (std::vector<std::string>{
                          "game layer shutdown",
                          "debug layers shutdown",
                          "stop stream consumed",
                          "stop stream consumed",
                          "audio teardown",
                      }));
    EXPECT_EQ(stopped_streams, (std::vector<u8>{5, 7}));
}

TEST(AudioStartup, MailboxStartsClosedAndOpensExplicitly) {
    audio::CommandMailbox mailbox{2};

    EXPECT_EQ(mailbox.tryPush(audio::cmd::StopStream{.id = 1}), audio::CommandSubmitResult::CLOSED);

    mailbox.open();

    EXPECT_EQ(mailbox.tryPush(audio::cmd::StopStream{.id = 1}), audio::CommandSubmitResult::ACCEPTED);
}

TEST(AudioStartup, ResetClearsQueueAndClosesAdmission) {
    audio::CommandMailbox mailbox{2};
    mailbox.open();
    ASSERT_EQ(mailbox.tryPush(audio::cmd::StopStream{.id = 1}), audio::CommandSubmitResult::ACCEPTED);

    mailbox.reset();

    EXPECT_TRUE(mailbox.empty());
    EXPECT_EQ(mailbox.tryPush(audio::cmd::StopStream{.id = 2}), audio::CommandSubmitResult::CLOSED);
}

TEST(AudioStartup, WorkerResultIsPublishedToWaitingInitializer) {
    audio::AudioThreadStartup startup;
    std::jthread worker([&] { startup.publish(Result::OAL_ERROR); });

    EXPECT_EQ(startup.wait(), Result::OAL_ERROR);
}

TEST(AudioStartup, SignalCanBeResetForAnotherInitialization) {
    audio::AudioThreadStartup startup;
    startup.publish(Result::OK);
    ASSERT_EQ(startup.wait(), Result::OK);

    startup.reset();
    std::jthread worker([&] { startup.publish(Result::OAL_ERROR); });

    EXPECT_EQ(startup.wait(), Result::OAL_ERROR);
}

TEST(AudioShutdown, RejectsCommandsAfterAdmissionCloses) {
    audio::CommandMailbox mailbox{2};
    mailbox.close();

    EXPECT_EQ(mailbox.tryPush(audio::cmd::StopStream{.id = 1}), audio::CommandSubmitResult::CLOSED);
    EXPECT_TRUE(mailbox.empty());
}

TEST(AudioShutdown, ReportsFullQueueWithoutClosingAdmission) {
    audio::CommandMailbox mailbox{2};
    mailbox.open();

    EXPECT_EQ(mailbox.tryPush(audio::cmd::StopStream{.id = 1}), audio::CommandSubmitResult::ACCEPTED);
    EXPECT_EQ(mailbox.tryPush(audio::cmd::StopStream{.id = 2}), audio::CommandSubmitResult::ACCEPTED);
    EXPECT_EQ(mailbox.tryPush(audio::cmd::StopStream{.id = 3}), audio::CommandSubmitResult::FULL);

    audio::Cmd popped;
    ASSERT_TRUE(mailbox.tryPop(popped));
    ASSERT_TRUE(std::holds_alternative<audio::cmd::StopStream>(popped));
    EXPECT_EQ(std::get<audio::cmd::StopStream>(popped).id, 1U);
    EXPECT_EQ(mailbox.tryPush(audio::cmd::StopStream{.id = 3}), audio::CommandSubmitResult::ACCEPTED);
}

TEST(AudioShutdown, EmptyDrainStillRunsTeardown) {
    audio::CommandMailbox mailbox{2};
    bool processed = false;
    bool torn_down = false;
    mailbox.close();

    audio::drainBeforeTeardown(mailbox, [&](const audio::Cmd&) { processed = true; }, [&] { torn_down = true; });

    EXPECT_FALSE(processed);
    EXPECT_TRUE(torn_down);
}
}  // namespace
}  // namespace mle
