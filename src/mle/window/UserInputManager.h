/**
 * @file
 * @brief User Input Management system for handling keyboard and mouse input.
 *
 * This file defines the `KeyListener` and `UserInputManager` classes that provide
 * keyboard and mouse input handling, text input listening, key modifiers,
 * and cursor tracking with normalized coordinates.
 */

#pragma once

#include <functional>
#include <utility>

#include "Types.h"
#include "mle/math/Types2D.h"
#include "mle/utils/Types.h"
#include "mle/utils/Utils.h"

namespace mle {
class KeyListener final {
  public:
    using CallbackFn = std::move_only_function<void()>;

  public:
    MLE_NO_COPY_MOVE(KeyListener);

    KeyListener(CallbackFn&& callback, Key key, KeyState state = KeyState::PRESSED, KeyModFlags mods = KeyModFlagBits::NONE) :
        callback_(std::move(callback)),
        key_(key),
        state_(state),
        mods_(mods) {
        listen();
    }

    KeyListener() = default;

    ~KeyListener() { unlisten(); }

    KeyListener& setCallback(CallbackFn&& callback);
    KeyListener& setKey(Key key);
    KeyListener& setState(KeyState state);
    KeyListener& setMods(KeyModFlags mods);

    [[nodiscard]] const auto& getCallback() const { return callback_; }
    [[nodiscard]] auto getKey() const { return key_; }
    [[nodiscard]] auto getState() const { return state_; }
    [[nodiscard]] auto getMods() const { return mods_; }

    [[nodiscard]] bool isSigned() const { return signed_; }

    void listen();
    void unlisten();

  private:
    friend UserInputManager;
    void setSigned(bool v = true) { signed_ = v; }

    [[nodiscard]] bool checkMods(bool shift, bool ctrl, bool alt) const;
    [[nodiscard]] bool tryCall(bool shift, bool ctrl, bool alt);
    void call() { callback_(); }

  private:
    CallbackFn callback_{};
    Key key_{};
    KeyState state_{};
    KeyModFlags mods_ = KeyModFlagBits::NONE;
    bool signed_ = false;
};

class UserInputManager {
    MLE_SINGLETON(UserInputManager)

  public:
    void update();
    void lateUpdate();

    [[nodiscard]] KeyState getState(Key key) const;
    [[nodiscard]] vec2f getCursorPos() const { return cursor_pos_; }
    vec2f getCursorPosNormalized() const { return cursor_pos_normalized_; }
    [[nodiscard]] vec2f getCursorDelta() const { return cursor_pos_delta_; }
    [[nodiscard]] f32 getScrollOffset() const { return scroll_offset_; }
    bool isPressed(Key key) const;
    bool isReleased(Key key) const;
    bool isDown(Key key) const;
    bool isCursorInside(const Rectf& rect) { return rect.contains(cursor_pos_); }
    bool isCursorInsideNormalized(const Rectf& rect) { return rect.contains(cursor_pos_normalized_); }

  private:
    friend Window;
    friend KeyListener;

    void init();

    void setCursorPos(const vec2d& pos);
    void setScrollOffset(f64 offset);

    void setPressed(Key key);
    void setReleased(Key key);

    void setKeyState(Key key, KeyState state);

    void pushChar(char32 codepoint);

    void listenKey(KeyListenerRef listener);
    void unlistenKey(KeyListenerRef listener);

    // ID listenText(std::function<void(char32)>&& callback);
    // void unlistenText(ID id);

  private:
    std::vector<std::pair<Key, KeyState>> active_keys_;
    std::unordered_map<u32, std::vector<KeyListenerRef>> listeners_;
    // IDVec<std::function<void(char32)>> text_listeners_;

    bool shift_ = false;
    bool ctrl_ = false;
    bool alt_ = false;

    vec2f cursor_pos_ = {nan<f32>(), nan<f32>()};
    vec2f cursor_pos_prev_ = {nan<f32>(), nan<f32>()};
    vec2f cursor_pos_delta_{0};

    vec2f cursor_pos_normalized_ = {nan<f32>(), nan<f32>()};
    vec2f cursor_pos_delta_normalized_ = {nan<f32>(), nan<f32>()};

    f32 scroll_offset_ = 0.0;
    f32 scroll_offset_next_ = 0.0;
};
}  // namespace mle
