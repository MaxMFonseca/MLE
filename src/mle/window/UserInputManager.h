/**
 * @file
 * @brief User Input Management system for handling keyboard and mouse input.
 *
 * This file defines the `KeyListener` and `UserInputManager` classes that provide
 * keyboard and mouse input handling, text input listening, key modifiers,
 * and cursor tracking with normalized coordinates.
 */

#pragma once

#include <chrono>
#include <functional>
#include <utility>

#include "Types.h"
#include "mle/math/Types2D.h"
#include "mle/utils/Stopwatch.h"
#include "mle/utils/Types.h"
#include "mle/utils/Utils.h"

// TODO: Maybe run this on a separate thread.
// along with window event handling
namespace mle {
class KeyListener final {
  public:
    using CallbackFn = std::move_only_function<void()>;

  public:
    MLE_NO_COPY_MOVE(KeyListener);

    KeyListener(CallbackFn&& callback, Keybinding kb) :
        callback_(std::move(callback)),
        kb_(kb) {}

    KeyListener(CallbackFn&& callback, Key key, KeyState state = KeyState::PRESSED, KeyModFlags mods = KeyModFlagBits::ANY) :
        KeyListener(std::move(callback), Keybinding{.key = key, .state = state, .mods = mods}) {}

    KeyListener() = default;

    ~KeyListener() { unlisten(); }

    KeyListener& setCallback(CallbackFn&& callback);
    KeyListener& setKey(Key key);
    KeyListener& setState(KeyState state);
    KeyListener& setMods(KeyModFlags mods);
    KeyListener& setKeybinding(Keybinding kb);

    [[nodiscard]] auto getKey() const { return kb_.key; }
    [[nodiscard]] auto getState() const { return kb_.state; }
    [[nodiscard]] auto getMods() const { return kb_.mods; }

    void listen();
    void unlisten();

    void setRepeat(bool enable) { repeat_ = enable; }

  private:
    friend UserInputManager;
    [[nodiscard]] bool checkMods(bool shift, bool ctrl, bool alt) const;
    [[nodiscard]] bool tryCall(bool shift, bool ctrl, bool alt);
    void call() { callback_(); }

  private:
    CallbackFn callback_{};
    Keybinding kb_{};
    bool repeat_ = false;
    bool listening_ = false;
};

class TextListener final {
  public:
    using CallbackFn = std::move_only_function<void(char32)>;

  public:
    MLE_NO_COPY_MOVE(TextListener);

    explicit TextListener(CallbackFn&& callback) :
        callback_(std::move(callback)) {}

    TextListener() = default;

    ~TextListener() { unlisten(); }

    TextListener& setCallback(CallbackFn&& callback);

    void listen();
    void unlisten();

  private:
    friend UserInputManager;
    void call(char32 codepoint) { callback_(codepoint); }

  private:
    CallbackFn callback_{};
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
    bool isCursorInside(const Rectf& rect) { return rect.intersect(cursor_pos_); }
    bool isCursorInsideNormalized(const Rectf& rect) { return rect.intersect(cursor_pos_normalized_); }

    bool isCtrl() const { return ctrl_; }
    bool isShift() const { return shift_; }
    bool isAlt() const { return alt_; }

  private:
    friend Window;
    friend KeyListener;
    friend TextListener;

    void init();

    void setCursorPos(const vec2d& pos);
    void setScrollOffset(f64 offset);

    void setPressed(Key key);
    void setReleased(Key key);

    void pushChar(char32 codepoint);

    void listenKey(KeyListenerRef listener);
    void unlistenKey(KeyListenerRef listener);

    void listenText(TextListenerRef listener);
    void unlistenText(TextListenerRef listener);

  private:
    std::vector<std::tuple<Key, KeyState, Stopwatch>> active_keys_;
    std::unordered_map<u32, std::vector<KeyListenerRef>> listeners_;
    std::vector<TextListenerRef> text_listeners_;

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

    f32 key_repeat_delay_s_ = 0.5F;
};
}  // namespace mle
