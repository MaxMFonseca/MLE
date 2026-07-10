#include <gtest/gtest.h>

#include <memory>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#define private public
#include "mle/window/UserInputManager.h"
#undef private
#pragma clang diagnostic pop

namespace {
class UserInputManagerTest : public ::testing::Test {
  protected:
    void TearDown() override {
        auto& uim = mle::UserInputManager::i();
        uim.active_keys_.clear();
        uim.listeners_.clear();
        uim.text_listeners_.clear();
        uim.scroll_listeners_.clear();
        uim.text_input_.clear();
    }
};
}  // namespace

TEST_F(UserInputManagerTest, AlwaysCallListenerRunsDespiteTopStackHandler) {
    int bottom_calls = 0;
    int always_calls = 0;
    int top_calls = 0;

    mle::KeyListener bottom([&] { ++bottom_calls; }, mle::Key::A);
    mle::KeyListener always([&] { ++always_calls; }, mle::Key::A);
    mle::KeyListener top([&] { ++top_calls; }, mle::Key::A);

    bottom.listen();
    always.setAlwaysCall(true).listen();
    top.listen();

    auto& uim = mle::UserInputManager::i();
    uim.setPressed(mle::Key::A);
    uim.update();

    EXPECT_EQ(bottom_calls, 0);
    EXPECT_EQ(always_calls, 1);
    EXPECT_EQ(top_calls, 1);

    uim.setReleased(mle::Key::A);
    uim.lateUpdate();
}

TEST_F(UserInputManagerTest, DispatchUsesListenerSnapshot) {
    int victim_calls = 0;
    int remover_calls = 0;
    mle::KeyListener victim([&] { ++victim_calls; }, mle::Key::ESCAPE);
    mle::KeyListener remover([&] {
        ++remover_calls;
        victim.unlisten();
    }, mle::Key::ESCAPE);
    victim.setAlwaysCall(true).listen();
    remover.setAlwaysCall(true).listen();

    auto& uim = mle::UserInputManager::i();
    uim.setPressed(mle::Key::ESCAPE);
    uim.update();

    EXPECT_EQ(remover_calls, 1);
    EXPECT_EQ(victim_calls, 1);
}

TEST_F(UserInputManagerTest, DispatchSnapshotSkipsListenerDestroyedByEarlierCallback) {
    int victim_calls = 0;
    int remover_calls = 0;
    auto victim = std::make_unique<mle::KeyListener>([&] { ++victim_calls; }, mle::Key::ESCAPE);
    mle::KeyListener remover([&] {
        ++remover_calls;
        victim.reset();
    }, mle::Key::ESCAPE);
    victim->setAlwaysCall(true).listen();
    remover.setAlwaysCall(true).listen();

    auto& uim = mle::UserInputManager::i();
    uim.setPressed(mle::Key::ESCAPE);
    uim.update();

    EXPECT_EQ(remover_calls, 1);
    EXPECT_EQ(victim_calls, 0);
}

TEST_F(UserInputManagerTest, ListenerCanUnlistenItselfDuringDispatch) {
    int calls = 0;
    mle::KeyListener listener;
    listener.setKey(mle::Key::ESCAPE).setState(mle::KeyState::PRESSED).setCallback([&] {
        ++calls;
        listener.unlisten();
    });
    listener.listen();

    auto& uim = mle::UserInputManager::i();
    uim.setPressed(mle::Key::ESCAPE);
    uim.update();
    EXPECT_EQ(calls, 1);

    uim.setReleased(mle::Key::ESCAPE);
    uim.lateUpdate();
    uim.setPressed(mle::Key::ESCAPE);
    uim.update();
    EXPECT_EQ(calls, 1);
}
