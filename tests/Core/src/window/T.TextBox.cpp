#include <gtest/gtest.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#define private public
#include "mle/window/UserInputManager.h"
#undef private
#pragma clang diagnostic pop

#include "mle/window/TextBox.h"

namespace {
class TextBoxTest : public ::testing::Test {
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

TEST_F(TextBoxTest, TerminalEscapePrecedenceModelRunsTerminalOnlyAfterTextBoxBlur) {
    int terminal_model_escape_calls = 0;
    mle::KeyListener terminal_model([&] { ++terminal_model_escape_calls; }, mle::Key::ESCAPE);
    terminal_model.listen();

    mle::TextBox text_box;
    text_box.setText(U"query");
    text_box.setSelection(1, 4);
    text_box.setFocused(true);

    auto& uim = mle::UserInputManager::i();
    uim.setPressed(mle::Key::ESCAPE);
    uim.update();

    EXPECT_FALSE(text_box.isFocused());
    EXPECT_EQ(text_box.getText(), U"query");
    EXPECT_FALSE(text_box.hasSelection());
    EXPECT_EQ(terminal_model_escape_calls, 0);

    uim.setReleased(mle::Key::ESCAPE);
    uim.lateUpdate();
    uim.setPressed(mle::Key::ESCAPE);
    uim.update();
    EXPECT_EQ(terminal_model_escape_calls, 1);
}

TEST_F(TextBoxTest, RefocusedTextBoxListensForEscapeAgain) {
    mle::TextBox text_box;
    auto& uim = mle::UserInputManager::i();

    text_box.setFocused(true);
    uim.setPressed(mle::Key::ESCAPE);
    uim.update();
    EXPECT_FALSE(text_box.isFocused());

    uim.setReleased(mle::Key::ESCAPE);
    uim.lateUpdate();

    text_box.setFocused(true);
    uim.setPressed(mle::Key::ESCAPE);
    uim.update();
    EXPECT_FALSE(text_box.isFocused());
}

TEST_F(TextBoxTest, FocusTransitionsNotifyChangedCallback) {
    int changes = 0;
    mle::TextBox text_box;
    text_box.setChangedCallback([&] { ++changes; });

    text_box.setFocused(true);
    EXPECT_EQ(changes, 1);

    text_box.setFocused(true);
    EXPECT_EQ(changes, 1);

    text_box.setFocused(false);
    EXPECT_EQ(changes, 2);

    text_box.setFocused(false);
    EXPECT_EQ(changes, 2);
}

TEST_F(TextBoxTest, SingleLineTextBoxDoesNotConsumeEnter) {
    int outer_calls = 0;
    mle::KeyListener outer([&] { ++outer_calls; }, mle::Key::ENTER);
    outer.listen();

    mle::TextBox text_box;
    text_box.setFocused(true);

    auto& uim = mle::UserInputManager::i();
    uim.setPressed(mle::Key::ENTER);
    uim.update();

    EXPECT_EQ(outer_calls, 1);
}
