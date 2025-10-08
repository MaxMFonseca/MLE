#include "TextBox.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cassert>
#include <cstring>

#include "Types.h"
#include "UserInputManager.h"

namespace mle {

TextBox::TextBox() {
    text_listener_ = std::make_unique<TextListener>([this](char32 codepoint) { onTextInput(codepoint); });
    backspace_kl_ = std::make_unique<KeyListener>([this]() { onBackspace(); }, Key::BACKSPACE, KeyState::PRESSED);
    backspace_kl_->setRepeat(true);
    delete_kl_ = std::make_unique<KeyListener>([this]() { onDelete(); }, Key::DELETE, KeyState::PRESSED);
    delete_kl_->setRepeat(true);
    left_kl_ = std::make_unique<KeyListener>([this]() { onLeft(); }, Key::LEFT, KeyState::PRESSED);
    left_kl_->setRepeat(true);
    right_kl_ = std::make_unique<KeyListener>([this]() { onRight(); }, Key::RIGHT, KeyState::PRESSED);
    right_kl_->setRepeat(true);
    home_kl_ = std::make_unique<KeyListener>([this]() { onHome(); }, Key::HOME, KeyState::PRESSED);
    end_kl_ = std::make_unique<KeyListener>([this]() { onEnd(); }, Key::END, KeyState::PRESSED);
    enter_kl_ = std::make_unique<KeyListener>([this]() { onEnter(); }, Key::ENTER, KeyState::PRESSED);
    end_kl_->setRepeat(true);
    ctrl_a_ = std::make_unique<KeyListener>([this]() { onCtrlA(); }, Key::A, KeyState::PRESSED, KeyModFlagBits::CTRL);
    ctrl_c_ = std::make_unique<KeyListener>([this]() { onCtrlC(); }, Key::C, KeyState::PRESSED, KeyModFlagBits::CTRL);
    ctrl_v_ = std::make_unique<KeyListener>([this]() { onCtrlV(); }, Key::V, KeyState::PRESSED, KeyModFlagBits::CTRL);
    ctrl_x_ = std::make_unique<KeyListener>([this]() { onCtrlX(); }, Key::X, KeyState::PRESSED, KeyModFlagBits::CTRL);
    // ctrl_z_ = std::make_unique<KeyListener>([this]() { onCtrlZ(); }, Key::Z, KeyState::PRESSED, KeyModFlagBits::CTRL);
}

void TextBox::onTextInput(char32 codepoint) {
    if (!focused_) {
        return;
    }

    insertChar(codepoint);
}

void TextBox::onBackspace() {
    if (!focused_) {
        return;
    }

    beginChange();
    if (selection_start_ != selection_end_) {
        deleteSelection();
    } else if (selection_start_ > 0) {
        text_.erase(selection_start_ - 1, 1);
        selection_start_--;
        selection_end_ = selection_start_;
    }
    endChange();
}

void TextBox::onDelete() {
    if (!focused_) {
        return;
    }

    beginChange();

    if (selection_start_ != selection_end_) {
        deleteSelection();
    } else if (selection_start_ < text_.size()) {
        text_.erase(selection_start_, 1);
    }

    selection_end_ = selection_start_;

    endChange();
}

void TextBox::onLeft() {
    if (!focused_) {
        return;
    }

    moveCursorLeft(UserInputManager::i().isShift());
}

void TextBox::onRight() {
    if (!focused_) {
        return;
    }

    moveCursorRight(UserInputManager::i().isShift());
}

void TextBox::onHome() {
    if (!focused_) {
        return;
    }

    moveCursorToStart(UserInputManager::i().isShift());
}

void TextBox::onEnd() {
    if (!focused_) {
        return;
    }

    moveCursorToEnd(UserInputManager::i().isShift());
}

void TextBox::onEnter() {
    if (!focused_) {
        return;
    }

    if (!new_line_ctrl_enter_ || UserInputManager::i().isCtrl()) {
        beginChange();
        insertChar(U'\n');
        selection_end_ = selection_start_;
        endChange();
    }
}

void TextBox::onCtrlA() {
    if (!focused_) {
        return;
    }

    beginChange();
    selection_start_ = 0;
    selection_end_ = text_.size();
    endChange();
}

void TextBox::onCtrlC() {
    if (!focused_) {
        return;
    }

    if (selection_start_ == selection_end_) {
        return;
    }

    MLE_ASSERT_LOG(selection_start_ <= text_.size(), "Selection start out of bounds");
    MLE_ASSERT_LOG(selection_end_ <= text_.size(), "Selection end out of bounds");
    MLE_ASSERT_LOG(selection_start_ <= selection_end_, "Selection start greater than end");

    auto selected = toUtf8(text_.substr(selection_start_, selection_end_ - selection_start_));
    SDL_SetClipboardText(selected.c_str());
}

void TextBox::onCtrlV() {
    if (!focused_) {
        return;
    }

    char* clip = SDL_GetClipboardText();
    if (clip) {
        beginChange();
        deleteSelection();
        insertText(clip);
        SDL_free(clip);
        endChange();
    }
}

void TextBox::onCtrlX() {
    if (!focused_) {
        return;
    }

    beginChange();
    onCtrlC();
    deleteSelection();
    endChange();
}

// void TextBox::onCtrlZ() {
// }

void TextBox::setFocused(bool focused) {
    focused_ = focused;

    if (!focused) {
        clearSelection();
    }
}

void TextBox::setSelection(usize start, usize end) {
    beginChange();
    selection_start_ = start;
    selection_end_ = end;
    endChange();
}

void TextBox::setNewLineCtrlEnter(bool enable) {
    new_line_ctrl_enter_ = enable;
}

void TextBox::deleteSelection() {
    if (selection_start_ == selection_end_) {
        return;
    }

    beginChange();
    usize start = std::min(selection_start_, selection_end_);
    usize end = std::max(selection_start_, selection_end_);
    text_.erase(start, end - start);
    selection_start_ = selection_end_ = start;
    endChange();
}

void TextBox::clearSelection() {
    beginChange();
    selection_end_ = selection_start_;
    endChange();
}

void TextBox::insertText(std::u32string_view text) {
    beginChange();
    deleteSelection();
    text_.insert(selection_start_, text);
    selection_start_ += text.size();
    selection_end_ = selection_start_;
    endChange();
}

void TextBox::insertText(std::string_view text) {
    insertText(toUtf32(text));
};

void TextBox::moveCursorLeft(bool select) {
    if (selection_start_ > 0) {
        beginChange();
        selection_start_--;
        if (!select) {
            selection_end_ = selection_start_;
        }
        endChange();
    }
}

void TextBox::moveCursorRight(bool select) {
    if (selection_end_ < text_.size()) {
        beginChange();
        selection_end_++;
        if (!select) {
            selection_start_ = selection_end_;
        }
        endChange();
    }
}

void TextBox::moveCursorToStart(bool select) {
    beginChange();
    selection_start_ = 0;
    if (!select) {
        selection_end_ = selection_start_;
    }
    endChange();
}

void TextBox::moveCursorToEnd(bool select) {
    beginChange();
    selection_start_ = text_.size();
    if (!select) {
        selection_end_ = selection_start_;
    }
    endChange();
}

void TextBox::setText(std::u32string_view text) {
    beginChange();
    text_ = text;
    selection_start_ = selection_end_ = text.size();
    endChange();
}

void TextBox::insertChar(char32 c) {
    beginChange();
    insertText(std::u32string_view(&c, 1));
    endChange();
}

std::string TextBox::makeSelectionString() {
    if (text_.empty()) {
        return "";
    }

    std::string text_utf8 = toUtf8(text_);
    std::string ret;

    for (usize i = 0; i <= text_utf8.size(); ++i) {
        if (i == selection_end_) {
            ret += '^';
        } else if (i >= selection_start_ && i < selection_end_) {
            ret += '|';
        } else {
            ret += ' ';
        }
    }

    return ret;
}

void TextBox::endChange() {
    MLE_ASSERT_LOG(changing_ > 0, "Mismatched beginChange/endChange calls!");

    changing_--;

    fixSelectionBounds();
    if (changing_ == 0) {
        if (changed_callback_) {
            changed_callback_();
        }
    }
}

void TextBox::fixSelectionBounds() {
    if (selection_start_ > selection_end_) {
        std::swap(selection_start_, selection_end_);
    }
    selection_start_ = std::min(selection_start_, text_.size());
    selection_end_ = std::min(selection_end_, text_.size());
}
}  // namespace mle
