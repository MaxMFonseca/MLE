#include "UserInputManager.h"

#include <bits/ranges_algo.h>

#include <algorithm>
#include <utility>

#include "mle/common/Utils.h"
#include "mle/window/Types.h"
#include "mle/window/Window.h"

namespace mle::window {
namespace {
bool areModsOk(const KeyModFlags& mods, bool is_pressed, KeyModFlagBits mod_bit, KeyModFlagBits mod_bit_no, KeyModFlagBits mod_bit_any) {
    if (mods.have(KeyModFlagBits::ANY)) {
        return true;
    }
    if (mods.have(KeyModFlagBits::NONE)) {
        return !is_pressed;
    }
    if (mods.have(mod_bit) && is_pressed) {
        return true;
    }
    if (mods.have(mod_bit_no) && !is_pressed) {
        return true;
    }
    if (mods.have(mod_bit_any)) {
        return true;
    }
    return false;
}
}  // namespace

bool KeyListener::checkMods(bool shift, bool ctrl, bool alt) const {
    bool shift_ok = areModsOk(mods_, shift, KeyModFlagBits::SHIFT, KeyModFlagBits::SHIFT_NO, KeyModFlagBits::SHIFT_ANY);
    bool ctrl_ok = areModsOk(mods_, ctrl, KeyModFlagBits::CTRL, KeyModFlagBits::CTRL_NO, KeyModFlagBits::CTRL_ANY);
    bool alt_ok = areModsOk(mods_, alt, KeyModFlagBits::ALT, KeyModFlagBits::ALT_NO, KeyModFlagBits::ALT_ANY);
    return shift_ok && ctrl_ok && alt_ok;
}

bool KeyListener::tryCall(bool shift, bool ctrl, bool alt) const {
    if (checkMods(shift, ctrl, alt)) {
        callback_();
        return true;
    }
    return false;
}

KeyListener& KeyListener::setCallback(CallbackFunction&& callback) {
    MLE_ASSERT(!signed_);
    callback_ = std::move(callback);
    return *this;
}
KeyListener& KeyListener::setKey(Key key) {
    MLE_ASSERT(!signed_);
    key_ = key;
    return *this;
}
KeyListener& KeyListener::setState(KeyState state) {
    MLE_ASSERT(!signed_);
    state_ = state;
    return *this;
}
KeyListener& KeyListener::setMods(KeyModFlags mods) {
    MLE_ASSERT(!signed_);
    mods_ = mods;
    return *this;
}
KeyListener& KeyListener::setPriority(int priority) {
    MLE_ASSERT(!signed_);
    priority_ = priority;
    return *this;
}

void KeyListener::sign() {
    getUIM().signKeylistener(this);
}
void KeyListener::unsign() {
    getUIM().unsignKeylistener(this);
}

void UserInputManager::lateUpdate() {
    auto it = active_keys_.begin();
    while (it != active_keys_.end()) {
        auto& [key, state] = *it;

        if (state == KeyState::PRESSED) {
            state = KeyState::DOWN;
        } else if (state == KeyState::RELEASED) {
            it = active_keys_.erase(it);
            continue;
        }
        ++it;
    }
}

void UserInputManager::update() {
    auto it = active_keys_.begin();
    while (it != active_keys_.end()) {
        auto& [key, state] = *it;

        auto listeners = listeners_.find(packKeyKeyState(key, state));
        if (listeners != listeners_.end()) {
            auto reverse_it = listeners->second.rbegin();
            for (; reverse_it != listeners->second.rend(); ++reverse_it) {
                if ((*reverse_it)->tryCall(shift_, ctrl_, alt_)) {
                    break;
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

    cursor_pos_normalized_ = cursor_pos_ / vec2f(window::getSize());
    cursor_pos_delta_normalized_ = cursor_pos_delta_ / vec2f(window::getSize());

    scroll_offset_ = scroll_offset_next_;
    scroll_offset_next_ = 0.0;

    cursor_pos_prev_ = cursor_pos_;
}

void UserInputManager::setCursorPos(const vec2d& pos) {
    cursor_pos_ = pos;
}

void UserInputManager::setScrollOffset(f64 offset) {
    scroll_offset_next_ = offset;
}

void UserInputManager::setPressed(Key key) {
    auto f = std::ranges::find_if(active_keys_, [&](const auto& p) { return p.first == key; });
    if (f == active_keys_.end()) {
        active_keys_.emplace_back(key, KeyState::PRESSED);
    } else {
        f->second = KeyState::PRESSED;
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
    auto it = std::ranges::find_if(active_keys_, [&](const auto& p) { return p.first == key; });
    MLE_ASSERT(it != active_keys_.end());
    it->second = KeyState::RELEASED;

    if (key == Key::LEFT_CONTROL || key == Key::RIGHT_CONTROL) {
        ctrl_ = false;
    } else if (key == Key::LEFT_SHIFT || key == Key::RIGHT_SHIFT) {
        shift_ = false;
    } else if (key == Key::LEFT_ALT || key == Key::RIGHT_ALT) {
        alt_ = false;
    }
}

void UserInputManager::pushChar(char32 codepoint) {
    for (const auto& fn : text_listeners_) {
        fn.second(codepoint);
    }
}

KeyState UserInputManager::getState(Key key) const {
    for (const auto& [k, state] : active_keys_) {
        if (k == key) {
            return state;
        }
    }
    return KeyState::UP;
}

void UserInputManager::setKeyState(Key key, KeyState state) {
    for (auto& [k, s] : active_keys_) {
        if (k == key) {
            s = state;
            return;
        }
    }
    active_keys_.emplace_back(key, state);
}

void UserInputManager::signKeylistener(KeyListenerRef listener) {
    auto& listeners = listeners_[packKeyKeyState(listener->getKey(), listener->getState())];
    MLE_ASSERT(std::ranges::find(listeners, listener) == listeners.end());
    MLE_ASSERT(!listener->isSigned());

    listeners.insert(std::ranges::lower_bound(listeners, listener, [](const auto& lhs, const auto& rhs) { return lhs->getPriority() <= rhs->getPriority(); }),
                     listener);
    listener->setSigned();
}

void UserInputManager::unsignKeylistener(KeyListenerRef listener) {
    auto& listeners = listeners_[packKeyKeyState(listener->getKey(), listener->getState())];
    std::erase(listeners, listener);
    listener->setSigned(false);
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
}  // namespace mle::window
