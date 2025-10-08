#include "UserInputManager.h"

#include <bits/ranges_algo.h>

#include <algorithm>
#include <utility>

#include "mle/window/KeyUtils.h"
#include "mle/window/Types.h"
#include "mle/window/Window.h"

namespace mle {
namespace {
bool isModOk(const KeyModFlags& mods, bool is_pressed, KeyModFlagBits mod_bit, KeyModFlagBits mod_bit_no) {
    if (mods.have(mod_bit)) {
        return is_pressed;
    }
    if (mods.have(mod_bit_no) || mods.have(KeyModFlagBits::NONE)) {
        return !is_pressed;
    }
    // Any is default
    return true;
}
}  // namespace

bool KeyListener::checkMods(bool shift, bool ctrl, bool alt) const {
    bool shift_ok = isModOk(kb_.mods, shift, KeyModFlagBits::SHIFT, KeyModFlagBits::SHIFT_NO);
    bool ctrl_ok = isModOk(kb_.mods, ctrl, KeyModFlagBits::CTRL, KeyModFlagBits::CTRL_NO);
    bool alt_ok = isModOk(kb_.mods, alt, KeyModFlagBits::ALT, KeyModFlagBits::ALT_NO);
    return shift_ok && ctrl_ok && alt_ok;
}

void KeyListener::tryCall(bool shift, bool ctrl, bool alt) {
    if (checkMods(shift, ctrl, alt)) {
        call();
    }
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

void UserInputManager::update() {
    auto it = active_keys_.begin();
    while (it != active_keys_.end()) {
        auto& [key, state, sw] = *it;

        auto listeners = listeners_.find(packKeyKeyState(key, state));
        if (listeners != listeners_.end()) {
            auto reverse_it = listeners->second.rbegin();
            for (; reverse_it != listeners->second.rend(); ++reverse_it) {
                auto& l = **reverse_it;
                l.tryCall(shift_, ctrl_, alt_);
            }
        }

        if (state == KeyState::DOWN && sw.elapsedSecFloat() > key_repeat_delay_s_) {
            auto listeners = listeners_.find(packKeyKeyState(key, KeyState::PRESSED));
            if (listeners != listeners_.end()) {
                auto reverse_it = listeners->second.rbegin();
                for (; reverse_it != listeners->second.rend(); ++reverse_it) {
                    auto& l = **reverse_it;
                    if (l.repeat_) {
                        l.tryCall(shift_, ctrl_, alt_);
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

bool UserInputManager::isPressed(Key key) const {
    return getState(key) == KeyState::PRESSED;
}

bool UserInputManager::isReleased(Key key) const {
    return getState(key) == KeyState::RELEASED;
}

bool UserInputManager::isDown(Key key) const {
    return getState(key) == KeyState::DOWN;
}

void UserInputManager::pushChar(char32 codepoint) {
    for (auto& listener : text_listeners_) {
        listener->call(codepoint);
    }
}
}  // namespace mle
