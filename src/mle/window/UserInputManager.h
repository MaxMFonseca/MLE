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
#include <map>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mle/common/Flags.h"
#include "mle/common/Types.h"
#include "mle/common/math/Types.h"
#include "mle/common/math/Types2D.h"
#include "mle/window/Types.h"
#include "mle/window/Utils.h"

namespace mle::window {
/// Listens for a specific key and invokes a callback when triggered.
class KeyListener final {
  public:
    /// Callback function signature.
    using CallbackFunction = std::function<void()>;

  public:
    KeyListener(const KeyListener&) = delete;
    KeyListener& operator=(const KeyListener&) = delete;
    KeyListener(KeyListener&&) = delete;
    KeyListener& operator=(KeyListener&&) = delete;

    /// Constructs a key listener and signs it.
    KeyListener(CallbackFunction&& callback, Key key, KeyState state = KeyState::PRESSED, KeyModFlags mods = KeyModFlagBits::NONE, int priority = 0) noexcept :
        callback_(std::move(callback)),
        key_(key),
        state_(state),
        mods_(mods),
        priority_(priority) {
        sign();
    }
    KeyListener() = default;  ///< Default constructor.

    ~KeyListener() { unsign(); }  ///< Destructor. Automatically unsigns.

    KeyListener& setCallback(CallbackFunction&& callback);  ///< Sets the callback function.
    KeyListener& setKey(Key key);                           ///< Sets the key to listen for.
    KeyListener& setState(KeyState state);                  ///< Sets the key state to listen for.
    KeyListener& setMods(KeyModFlags mods);                 ///< Sets the key modifier flags.
    KeyListener& setPriority(int priority);                 /// Sets the priority of the listener.

    [[nodiscard]] auto getCallback() const { return callback_; }  ///< Returns the callback function.
    [[nodiscard]] auto getKey() const { return key_; }            ///< Returns the key being listened for.
    [[nodiscard]] auto getState() const { return state_; }        ///< Returns the key state being listened for.
    [[nodiscard]] auto getMods() const { return mods_; }          ///< Returns the key modifier flags.
    [[nodiscard]] auto getPriority() const { return priority_; }  ///< Returns the priority of the listener.

    [[nodiscard]] bool isSigned() const { return signed_; }  ///< Returns whether the listener is signed.

    [[nodiscard]] bool checkMods(bool shift, bool ctrl, bool alt) const;  ///< Checks if the given modifier state matches this listener.
    [[nodiscard]] bool tryCall(bool shift, bool ctrl, bool alt) const;    ///< Attempts to call the callback function if the key state matches.

    void sign();    ///< Signs the listener.
    void unsign();  ///< Unsings the listener.

  private:
    friend class UserInputManager;                  ///< Friend class to allow access to private members.
    void setSigned(bool v = true) { signed_ = v; }  ///< Returns whether the listener is signed.

  private:
    CallbackFunction callback_{};              ///< Callback to invoke on match.
    Key key_{};                                ///< Key being listened for.
    KeyState state_{};                         ///< Key state to match.
    KeyModFlags mods_ = KeyModFlagBits::NONE;  ///< Required key modifiers.
    int priority_ = 0;                         ///< Execution priority.
    bool signed_ = false;                      ///< Whether the listener is active.
};

/// Central input manager that handles key, mouse, scroll, and text input.
class UserInputManager {
  public:
    UserInputManager(const UserInputManager&) = delete;
    UserInputManager& operator=(const UserInputManager&) = delete;
    UserInputManager(UserInputManager&&) = delete;
    UserInputManager& operator=(UserInputManager&&) = delete;

    /// Constructs a new input manager.
    UserInputManager() = default;

    UserInputManager(std::vector<std::pair<Key, KeyState>> active_keys, std::unordered_map<u32, std::vector<KeyListenerRef>> listeners,
                     IDVec<std::function<void(char32)>> text_listeners) :
        active_keys_(std::move(active_keys)),
        listeners_(std::move(listeners)),
        text_listeners_(std::move(text_listeners)) {}
    /// Destructor.
    ~UserInputManager() = default;

    /// Updates internal state each frame.
    void update();

    /// Updates internal state on frame end.
    void lateUpdate();

    /// Sets the current cursor position in screen space.
    void setCursorPos(const vec2d& pos);

    /// Sets the scroll wheel offset.
    void setScrollOffset(f64 offset);

    /// Marks a key as pressed.
    void setPressed(Key key);

    /// Marks a key as released.
    void setReleased(Key key);

    /// Pushes a typed Unicode character to listeners.
    void pushChar(char32 codepoint);

    /// Returns the current state of the given key.
    [[nodiscard]] KeyState getState(Key key) const;

    /// Returns the current cursor position in screen space.
    [[nodiscard]] vec2f getCursorPos() const { return cursor_pos_; }

    /// Returns the current normalized cursor position.
    vec2f getCursorPosNormalized() const { return cursor_pos_normalized_; }

    /// Returns the screen-space cursor delta since the last frame.
    [[nodiscard]] vec2f getCursorDelta() const { return cursor_pos_delta_; }

    /// Returns the current scroll offset.
    [[nodiscard]] f64 getScrollOffset() const { return scroll_offset_; }

    /// Returns true if the key is in the pressed state.
    bool isPressed(Key key) const;

    /// Returns true if the key is in the released state.
    bool isReleased(Key key) const;

    /// Returns true if the key is held down.
    bool isDown(Key key) const;

    /// Returns true if the cursor is inside the given rect (in screen space).
    bool isCursorInside(const Rectf& rect) { return rect.contains(cursor_pos_); }

    /// Returns true if the cursor is inside the given rect (in normalized space).
    bool isCursorInsideNormalized(const Rectf& rect) { return rect.contains(cursor_pos_normalized_); }

    /// Signs a key listener.
    void signKeylistener(KeyListenerRef listener);

    /// Unsigns a key listener.
    void unsignKeylistener(KeyListenerRef listener);

    /**
     * @brief Adds a UTF-32 character input listener.
     *
     * @param callback Function to invoke when a character is typed.
     * @return A unique ID used to remove the listener later.
     * @note The callback receives a single `char32_t` codepoint.
     */
    ID addTextListener(std::function<void(char32)>&& callback);

    /// Removes a text input listener by ID.
    void removeTextListener(ID id);

  private:
    /// Sets the state of a key.
    void setKeyState(Key key, KeyState state);

  private:
    std::vector<std::pair<Key, KeyState>> active_keys_;               ///< Active key states.
    std::unordered_map<u32, std::vector<KeyListenerRef>> listeners_;  ///< Map of key to listeners.
    IDVec<std::function<void(char32)>> text_listeners_;               ///< Text input listeners.

    bool shift_ = false;  ///< Current shift state.
    bool ctrl_ = false;   ///< Current ctrl state.
    bool alt_ = false;    ///< Current alt state.

    vec2f cursor_pos_ = {nan<f32>(), nan<f32>()};       ///< Current cursor position.
    vec2f cursor_pos_prev_ = {nan<f32>(), nan<f32>()};  ///< Previous cursor position.
    vec2f cursor_pos_delta_{0};

    vec2f cursor_pos_normalized_ = {nan<f32>(), nan<f32>()};        ///< Normalized cursor position.
    vec2f cursor_pos_delta_normalized_ = {nan<f32>(), nan<f32>()};  ///< Normalized cursor delta.

    f64 scroll_offset_ = 0.0;       ///< Current scroll offset.
    f64 scroll_offset_next_ = 0.0;  ///< Scroll offset accumulator.
};
}  // namespace mle::window
