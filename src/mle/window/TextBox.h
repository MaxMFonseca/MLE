#pragma once

#include <stack>

#include "mle/utils/Stopwatch.h"
#include "mle/utils/String.h"
#include "mle/utils/Utils.h"
#include "mle/window/Types.h"
#include "mle/window/UserInputManager.h"

namespace mle {
class TextBox final {
  public:
    MLE_NO_COPY_MOVE(TextBox)

    TextBox();
    ~TextBox() = default;

    void setFocused(bool focused);
    [[nodiscard]] bool isFocused() const { return focused_; }

    void setText(std::u32string_view text);
    [[nodiscard]] const std::u32string& getText() const { return text_; }
    [[nodiscard]] std::string getTextUtf8() const { return toUtf8(text_); }

    [[nodiscard]] usize getSelectionStart() const { return selection_start_; }
    [[nodiscard]] usize getSelectionEnd() const { return selection_end_; }
    [[nodiscard]] std::pair<usize, usize> getSelection() const { return {selection_start_, selection_end_}; }
    [[nodiscard]] bool hasSelection() const { return selection_start_ != selection_end_; }

    void setSelection(usize start, usize end);

    void setChangedCallback(std::move_only_function<void()>&& callback) { changed_callback_ = std::move(callback); }

    void setNewLineCtrlEnter(bool enable);
    void setAllowNewLine(bool allow) { allow_new_line_ = allow; }

    std::string makeSelectionString();

  private:
    void onTextInput(char32 codepoint);
    void onBackspace();
    void onDelete();
    void onLeft();
    void onRight();
    void onHome();
    void onEnd();
    void onEnter();
    void onCtrlA();
    void onCtrlC();
    void onCtrlV();
    void onCtrlX();
    // void onCtrlZ();

    void insertText(std::u32string_view text);
    void insertText(std::string_view text);
    void insertChar(char32 c);

    void deleteSelection();
    void clearSelection();
    void moveCursorLeft(bool select);
    void moveCursorRight(bool select);
    void moveCursorToStart(bool select);
    void moveCursorToEnd(bool select);

    void beginChange() { changing_++; }
    void endChange();
    void fixSelectionBounds();

  private:
    std::u32string text_;
    int changing_ = 0;

    usize selection_start_ = 0;
    usize selection_end_ = 0;
    bool focused_ = false;
    bool allow_new_line_ = false;
    bool new_line_ctrl_enter_ = false;

    std::move_only_function<void()> changed_callback_;

    TextListener text_listener_;

    KeyListener backspace_kl_;
    KeyListener delete_kl_;
    KeyListener left_kl_;
    KeyListener right_kl_;
    KeyListener home_kl_;
    KeyListener end_kl_;
    KeyListener enter_kl_;
    KeyListener ctrl_a_;
    KeyListener ctrl_c_;
    KeyListener ctrl_v_;
    KeyListener ctrl_x_;
    // KeyListenerHnd ctrl_z_;
};
}  // namespace mle
