/**
 * @file
 * @brief Input-related type definitions and flags for window interaction.
 *
 * Defines key states, modifiers, and key codes used throughout the engine,
 * including mouse and special keys.
 */

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "mle/common/Flags.h"
#include "mle/common/math/Types.h"

namespace mle::window {
/// Non-owning pointer to a GLFW window.
using GLFWWindowRef = GLFWwindow*;

class UserInputManager;

/// Abstract listener for key input events.
class KeyListener;

/// Owning handle to a key listener.
using KeyListenerHnd = std::unique_ptr<KeyListener>;

/// Non-owning reference to a key listener.
using KeyListenerRef = KeyListener*;

/// Describes the current state of a key or button.
enum class KeyState : u8 {
    UP,       ///< Key was up last frame and is still up.
    PRESSED,  ///< Key transitioned from up to down this frame.
    DOWN,     ///< Key is held down.
    RELEASED  ///< Key transitioned from down to up this frame.
};

/// Modifier keys used during key input events.
MLE_FLAGS_BEGIN(KeyMod, u16)
MLE_FLAG(KeyMod, SHIFT)      ///< Shift key held.
MLE_FLAG(KeyMod, CTRL)       ///< Control key held.
MLE_FLAG(KeyMod, ALT)        ///< Alt key held.
MLE_FLAG(KeyMod, SHIFT_NO)   ///< Shift key not held.
MLE_FLAG(KeyMod, CTRL_NO)    ///< Control key not held.
MLE_FLAG(KeyMod, ALT_NO)     ///< Alt key not held.
MLE_FLAG(KeyMod, SHIFT_ANY)  ///< Shift is either held or not (matches any).
MLE_FLAG(KeyMod, CTRL_ANY)   ///< Control is either held or not (matches any).
MLE_FLAG(KeyMod, ALT_ANY)    ///< Alt is either held or not (matches any).
MLE_FLAG(KeyMod, NONE)       ///< No modifiers.
MLE_FLAG(KeyMod, ANY)        ///< Any combination of modifiers.
MLE_FLAGS_END(KeyMod);

// clang-format off

/// Keyboard and mouse key identifiers.
enum class Key : u8 {  // NOLINT
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, ZERO,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    RIGHT, LEFT, DOWN, UP,
    LEFT_SHIFT, LEFT_CONTROL, LEFT_ALT, LEFT_SUPER,
    RIGHT_SHIFT, RIGHT_CONTROL, RIGHT_ALT, RIGHT_SUPER,
    ESCAPE, SPACE, ENTER, TAB, BACKSPACE, INSERT, DELETE, PAGE_UP, PAGE_DOWN,
    HOME, END, CAPS_LOCK, SCROLL_LOCK, NUM_LOCK, PRINT_SCREEN, PAUSE, MENU, 
    GRAVE_ACCENT, MINUS, EQUAL, LEFT_BRACKET, RIGHT_BRACKET, SEMICOLON, APOSTROPHE,

    MOUSE_ONE,
    MOUSE_LEFT = MOUSE_ONE,
    MOUSE_RIGHT,
    MOUSE_MIDDLE,
    MOUSE_FOUR, MOUSE_FIVE, MOUSE_SIX, MOUSE_SEVEN, MOUSE_EIGHT,

    COUNT,
    UNKNOWN
};
// clang-format on

}  // namespace mle::window

namespace fmt {
template <>
struct formatter<mle::window::KeyState> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::window::KeyState s, FormatContext& ctx) const {
        using mle::window::KeyState;
        switch (s) {
            case KeyState::UP:
                return format_to(ctx.out(), "UP");
            case KeyState::PRESSED:
                return format_to(ctx.out(), "PRESSED");
            case KeyState::DOWN:
                return format_to(ctx.out(), "DOWN");
            case KeyState::RELEASED:
                return format_to(ctx.out(), "RELEASED");
            default:
                return format_to(ctx.out(), "UNKNOWN");
        }
    }
};
}  // namespace fmt

MLE_FLAGS_FMT_BEGIN(mle::window::KeyMod)
MLE_FLAGS_FMT_CASE(mle::window::KeyMod, SHIFT)
MLE_FLAGS_FMT_CASE(mle::window::KeyMod, CTRL)
MLE_FLAGS_FMT_CASE(mle::window::KeyMod, ALT)
MLE_FLAGS_FMT_CASE(mle::window::KeyMod, SHIFT_NO)
MLE_FLAGS_FMT_CASE(mle::window::KeyMod, CTRL_NO)
MLE_FLAGS_FMT_CASE(mle::window::KeyMod, ALT_NO)
MLE_FLAGS_FMT_CASE(mle::window::KeyMod, SHIFT_ANY)
MLE_FLAGS_FMT_CASE(mle::window::KeyMod, CTRL_ANY)
MLE_FLAGS_FMT_CASE(mle::window::KeyMod, ALT_ANY)
MLE_FLAGS_FMT_CASE(mle::window::KeyMod, NONE)
MLE_FLAGS_FMT_CASE(mle::window::KeyMod, ANY)
MLE_FLAGS_FMT_END(mle::window::KeyMod);
