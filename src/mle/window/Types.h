#pragma once

#include <memory>

#include "mle/math/Types.h"
#include "mle/utils/Flags.h"

namespace mle {
class Window;

class UserInputManager;

class KeyListener;
using KeyListenerHnd = std::unique_ptr<KeyListener>;
using KeyListenerRef = KeyListener*;

class TextListener;
using TextListenerHnd = std::unique_ptr<TextListener>;
using TextListenerRef = TextListener*;

class TextBox;
using TextBoxRef = TextBox*;
using TextBoxHnd = std::unique_ptr<TextBox>;

enum class KeyState : u8 {
    UP,
    PRESSED,
    DOWN,
    RELEASED,
};

MLE_FLAGS_BEGIN(KeyMod, u8)
MLE_FLAG(KeyMod, SHIFT)
MLE_FLAG(KeyMod, CTRL)
MLE_FLAG(KeyMod, ALT)
MLE_FLAG(KeyMod, NONE)
MLE_FLAG(KeyMod, ANY)
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
    COMMA, PERIOD, SLASH, BACKSLASH,

    MOUSE_ONE,
    MOUSE_LEFT = MOUSE_ONE,
    MOUSE_RIGHT,
    MOUSE_MIDDLE,
    MOUSE_FOUR, MOUSE_FIVE, MOUSE_SIX, MOUSE_SEVEN, MOUSE_EIGHT,

    COUNT,
    UNKNOWN
};
// clang-format on

struct Keybinding {
    Key key{Key::UNKNOWN};
    KeyState state{KeyState::PRESSED};
    KeyModFlags mods{KeyModFlagBits::NONE};
};
}  // namespace mle

namespace fmt {
template <>
struct formatter<mle::KeyState> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::KeyState s, FormatContext& ctx) const {
        switch (s) {
            case mle::KeyState::UP:
                return format_to(ctx.out(), "UP");
            case mle::KeyState::PRESSED:
                return format_to(ctx.out(), "PRESSED");
            case mle::KeyState::DOWN:
                return format_to(ctx.out(), "DOWN");
            case mle::KeyState::RELEASED:
                return format_to(ctx.out(), "RELEASED");
            default:
                return format_to(ctx.out(), "UNKNOWN");
        }
    }
};

template <>
struct formatter<mle::Keybinding> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::Keybinding& kb, FormatContext& ctx) const {
        return format_to(ctx.out(), "(key: {}, state: {}, mods: {})", as<mle::u32>(kb.key), kb.state, kb.mods);
    }
};

}  // namespace fmt

MLE_FLAGS_FMT_BEGIN(mle::KeyMod)
MLE_FLAGS_FMT_CASE(mle::KeyMod, SHIFT)
MLE_FLAGS_FMT_CASE(mle::KeyMod, CTRL)
MLE_FLAGS_FMT_CASE(mle::KeyMod, ALT)
MLE_FLAGS_FMT_CASE(mle::KeyMod, NONE)
MLE_FLAGS_FMT_CASE(mle::KeyMod, ANY)
MLE_FLAGS_FMT_END(mle::KeyMod);
