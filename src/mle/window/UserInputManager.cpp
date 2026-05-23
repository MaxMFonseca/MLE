#include "UserInputManager.h"

#include <bits/ranges_algo.h>

#include <algorithm>
#include <utility>

#include "mle/core/Result.h"
#include "mle/window/KeyUtils.h"
#include "mle/window/Types.h"
#include "mle/window/Window.h"

namespace mle {
bool KeyListener::checkMods(bool shift, bool ctrl, bool alt) const {
    if (kb_.mods & KeyModFlagBits::ANY) {
        return true;
    }
    if (kb_.mods & KeyModFlagBits::NONE) {
        return !(shift || ctrl || alt);
    }
    bool alt_ok = !as<bool>(as<bool>(kb_.mods & KeyModFlagBits::ALT) ^ alt);
    bool ctrl_ok = !as<bool>(as<bool>(kb_.mods & KeyModFlagBits::CTRL) ^ ctrl);
    bool shift_ok = !as<bool>(as<bool>(kb_.mods & KeyModFlagBits::SHIFT) ^ shift);
    return alt_ok && ctrl_ok && shift_ok;
}

bool KeyListener::tryCall(bool shift, bool ctrl, bool alt, bool text_active) {
    if (checkMods(shift, ctrl, alt)) {
        if (!text_active) {
            call();
            return true;
        }

        bool is_ctrl_alt = as<bool>(kb_.mods & (KeyModFlagBits::CTRL | KeyModFlagBits::ALT));

        if (is_ctrl_alt || isMouseKey(kb_.key) || !isPrintableKey(kb_.key)) {
            call();
            return true;
        }
    }
    return false;
}

KeyListener& KeyListener::setCallback(CallbackFn&& callback) {
    unlisten();
    callback_ = std::move(callback);
    return *this;
}

KeyListener& KeyListener::setKey(Key key) {
    unlisten();
    kb_.key = key;
    return *this;
}

KeyListener& KeyListener::setState(KeyState state) {
    unlisten();
    kb_.state = state;
    return *this;
}

KeyListener& KeyListener::setMods(KeyModFlags mods) {
    unlisten();
    kb_.mods = mods;
    return *this;
}

KeyListener& KeyListener::setKeybinding(Keybinding kb) {
    unlisten();
    kb_ = kb;
    return *this;
}

KeyListener& KeyListener::setRepeat(bool enable) {
    repeat_ = enable;
    return *this;
}

void KeyListener::listen() {
    if (listening_) {
        return;
    }
    UserInputManager::i().listenKey(this);
    listening_ = true;
}

void KeyListener::unlisten() {
    if (!listening_) {
        return;
    }
    UserInputManager::i().unlistenKey(this);
    listening_ = false;
}

TextListener& TextListener::setCallback(CallbackFn&& callback) {
    callback_ = std::move(callback);
    return *this;
}

void TextListener::listen() {
    UserInputManager::i().listenText(this);
}

void TextListener::unlisten() {
    UserInputManager::i().unlistenText(this);
}

ScrollListener& ScrollListener::setCallback(CallbackFn&& callback) {
    callback_ = std::move(callback);
    return *this;
}

void ScrollListener::listen() {
    if (listening_) {
        return;
    }
    UserInputManager::i().listenScroll(this);
    listening_ = true;
}

void ScrollListener::unlisten() {
    if (!listening_) {
        return;
    }
    UserInputManager::i().unlistenScroll(this);
    listening_ = false;
}

void UserInputManager::update() {
    auto it = active_keys_.begin();

    bool text_active = !text_listeners_.empty();

    while (it != active_keys_.end()) {
        auto& [key, state, sw] = *it;

        auto listeners = listeners_.find(packKeyKeyState(key, state));
        if (listeners != listeners_.end()) {
            auto reverse_it = listeners->second.rbegin();
            for (; reverse_it != listeners->second.rend(); ++reverse_it) {
                auto& l = **reverse_it;
                if (l.tryCall(shift_, ctrl_, alt_, text_active)) {
                    break;
                }
            }
        }

        if (state == KeyState::DOWN && sw.elapsedSecFloat() > key_repeat_delay_s_) {
            auto listeners = listeners_.find(packKeyKeyState(key, KeyState::PRESSED));
            if (listeners != listeners_.end()) {
                auto reverse_it = listeners->second.rbegin();
                for (; reverse_it != listeners->second.rend(); ++reverse_it) {
                    auto& l = **reverse_it;
                    if (l.repeat_) {
                        if (l.tryCall(shift_, ctrl_, alt_, text_active)) {
                            break;
                        }
                    }
                }
            }
        }

        it++;
    }

    if (!std::isnan(cursor_pos_.x) && !std::isnan(cursor_pos_prev_.x)) {
        cursor_pos_delta_ = cursor_pos_ - cursor_pos_prev_;
    } else {
        cursor_pos_delta_ = {0.0, 0.0};
    }

    auto window_size = Window::i().getSize();
    auto window_size_f = vec2f(window_size);
    cursor_pos_normalized_ = cursor_pos_ / window_size_f;
    cursor_pos_delta_normalized_ = cursor_pos_delta_ / window_size_f;

    scroll_offset_ = scroll_offset_next_;
    scroll_offset_next_ = 0.0;
    if (scroll_offset_ != 0.0F) {
        for (auto& listener : scroll_listeners_) {
            listener->call(scroll_offset_);
        }
    }

    cursor_pos_prev_ = cursor_pos_;
}

void UserInputManager::lateUpdate() {
    auto it = active_keys_.begin();
    while (it != active_keys_.end()) {
        auto& [key, state, sw] = *it;

        if (state == KeyState::PRESSED) {
            state = KeyState::DOWN;
        } else if (state == KeyState::RELEASED) {
            it = active_keys_.erase(it);
            continue;
        }
        ++it;
    }
}

void UserInputManager::setCursorPos(const vec2d& pos) {
    cursor_pos_ = pos;
}

void UserInputManager::setScrollOffset(f64 offset) {
    scroll_offset_next_ = as<f32>(offset);
}

void UserInputManager::setPressed(Key key) {
    auto f = std::ranges::find_if(active_keys_, [&](const auto& p) { return std::get<0>(p) == key; });
    if (f == active_keys_.end()) {
        active_keys_.emplace_back(key, KeyState::PRESSED, Stopwatch{});
    } else if (std::get<1>(*f) != KeyState::DOWN) {
        std::get<1>(*f) = KeyState::PRESSED;
    }

    if (key == Key::LEFT_CONTROL || key == Key::RIGHT_CONTROL) {
        ctrl_ = true;
    } else if (key == Key::LEFT_SHIFT || key == Key::RIGHT_SHIFT) {
        shift_ = true;
    } else if (key == Key::LEFT_ALT || key == Key::RIGHT_ALT) {
        alt_ = true;
    }
}

void UserInputManager::setReleased(Key key) {
    auto it = std::ranges::find_if(active_keys_, [&](const auto& p) { return std::get<0>(p) == key; });
    MLE_ASSERT(it != active_keys_.end());
    std::get<1>(*it) = KeyState::RELEASED;

    if (key == Key::LEFT_CONTROL || key == Key::RIGHT_CONTROL) {
        ctrl_ = false;
    } else if (key == Key::LEFT_SHIFT || key == Key::RIGHT_SHIFT) {
        shift_ = false;
    } else if (key == Key::LEFT_ALT || key == Key::RIGHT_ALT) {
        alt_ = false;
    }
}

KeyState UserInputManager::getState(Key key) const {
    for (const auto& [k, state, sw] : active_keys_) {
        if (k == key) {
            return state;
        }
    }
    return KeyState::UP;
}

void UserInputManager::listenKey(KeyListenerRef listener) {
    auto& listeners = listeners_[packKeyKeyState(listener->getKey(), listener->getState())];
    auto it = std::ranges::find(listeners, listener);
    if (it == listeners.end()) {
        listeners.push_back(listener);
    }
}

void UserInputManager::unlistenKey(KeyListenerRef listener) {
    auto& listeners = listeners_[packKeyKeyState(listener->getKey(), listener->getState())];
    std::erase(listeners, listener);
}

void UserInputManager::listenText(TextListenerRef listener) {
    auto it = std::ranges::find(text_listeners_, listener);
    MLE_ASSERT(it == text_listeners_.end());
    text_listeners_.push_back(listener);
}

void UserInputManager::unlistenText(TextListenerRef listener) {
    std::erase(text_listeners_, listener);
}

void UserInputManager::listenScroll(ScrollListenerRef listener) {
    auto it = std::ranges::find(scroll_listeners_, listener);
    if (it == scroll_listeners_.end()) {
        scroll_listeners_.push_back(listener);
    }
}

void UserInputManager::unlistenScroll(ScrollListenerRef listener) {
    std::erase(scroll_listeners_, listener);
}

bool UserInputManager::isPressed(Key key) const {
    return getState(key) == KeyState::PRESSED;
}

bool UserInputManager::isReleased(Key key) const {
    return getState(key) == KeyState::RELEASED;
}

bool UserInputManager::isDown(Key key) const {
    auto state = getState(key);
    return state == KeyState::DOWN || state == KeyState::PRESSED;
}

void UserInputManager::pushChar(char32 codepoint) {
    for (auto& listener : text_listeners_) {
        listener->call(codepoint);
    }
}

void UserInputManager::setMouseInsideWindow(bool inside) {
    mouse_inside_window_ = inside;

    if (inside == false) {
        cursor_pos_ = {NAN, NAN};
        cursor_pos_normalized_ = {NAN, NAN};
        cursor_pos_delta_ = {0.0, 0.0};
        cursor_pos_delta_normalized_ = {0.0, 0.0};
        scroll_offset_ = 0.0;
        scroll_offset_next_ = 0.0;
    }
}

[[nodiscard]] Expected<vec2f> UserInputManager::getCursorPos() const {
    if (!mouse_inside_window_) {
        return std::unexpected(Result::CURSOR_NOT_INSIDE_WINDOW);
    }
    return cursor_pos_;
}

[[nodiscard]] Expected<vec2f> UserInputManager::getCursorPosNormalized() const {
    if (!mouse_inside_window_) {
        return std::unexpected(Result::CURSOR_NOT_INSIDE_WINDOW);
    }
    return cursor_pos_normalized_;
}

[[nodiscard]] Expected<vec2f> UserInputManager::getCursorDelta() const {
    if (!mouse_inside_window_) {
        return std::unexpected(Result::CURSOR_NOT_INSIDE_WINDOW);
    }
    return cursor_pos_delta_;
}

[[nodiscard]] Expected<f32> UserInputManager::getScrollOffset() const {
    if (!mouse_inside_window_) {
        return std::unexpected(Result::CURSOR_NOT_INSIDE_WINDOW);
    }
    return scroll_offset_;
}

bool UserInputManager::isCursorInside(const Rectf& rect) {
    if (!mouse_inside_window_) {
        return false;
    }
    return rect.intersect(cursor_pos_);
}

bool UserInputManager::isCursorInsideNormalized(const Rectf& rect) {
    if (!mouse_inside_window_) {
        return false;
    }
    return rect.intersect(cursor_pos_normalized_);
}
}  // namespace mle
