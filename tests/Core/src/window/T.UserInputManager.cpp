#include <gtest/gtest.h>

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
