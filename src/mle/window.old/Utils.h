/**
 * @file Utils.h
 * @brief Input utility functions for mapping GLFW values and parsing string key bindings.
 */

#pragma once

#include "mle/common/Result.h"
#include "mle/window/Types.h"

namespace mle::window {
/// Converts a GLFW key code to an engine Key.
Key castGlfwKeyToKey(i32 code);

/// Converts a GLFW mouse button index to a Key (MOUSE_*).
Key castGlfwMouseButtonToKey(i32 code) {
    return Key(static_cast<i32>(Key::MOUSE_ONE) + code);
}

/// Packs a key and key state into a u32 value.
inline u32 packKeyKeyState(Key key, KeyState state) {
    return static_cast<u32>(key) | (static_cast<u32>(state) << 8U);
}

/**
 * @brief Parses a key binding string into a (Key, KeyState, KeyModFlags) tuple.
 *
 * This function decodes a compact string representation of a key binding,
 * typically used in input mappings or scripting contexts. The string is
 * expected to follow the format:
 *
 * ```
 * "<key>[:<state>[:<modifiers>]]"
 * ```
 *
 * - `<key>`: The key name (e.g., `"a"`, `"enter"`, `"f1"`, `"lmb"`, etc.)
 * - `<state>` (optional): A single character representing the desired key state:
 *   - `'p'` = PRESSED (default)
 *   - `'d'` = DOWN
 *   - `'u'` = UP
 *   - `'r'` = RELEASED
 * - `<modifiers>` (optional): Modifier flags as a compact string interpreted by `toKeyModFlags()`
 *   - Examples: `"s"` for Shift, `"yn*"` for Shift required, Ctrl forbidden, Alt any
 *
 * If the key string is invalid, an error is returned.
 * If state or modifiers are omitted, defaults are applied.
 *
 * @param str A string representing a key binding, such as `"a"`, `"enter:p"`, or `"f5:r:s"`.
 * @return A std::tuple<Key, KeyState, KeyModFlags> with the parsed result, or an error if the key part is invalid.
 *
 * @see toKey, toKeyState, toKeyModFlags
 *
 * @code
 * toKeyTuple("a");         // ('A', PRESSED, NONE)
 * toKeyTuple("space:r");   // (SPACE, RELEASED, NONE)
 * toKeyTuple("f5:d:yn0");  // (F5, DOWN, SHIFT | CTRL_NO | ALT_NO)
 * toKeyTuple("enter:*:*"); // (ENTER, PRESSED, ANY | ANY | ANY)
 * @endcode
 */
Expected<std::tuple<Key, KeyState, KeyModFlags>> toKeyTuple(const std::string& str);

/**
 * @brief Converts a string to a Key enum value.
 *
 * This function maps string identifiers (e.g., "a", "F5", "enter", "lmb") to their corresponding
 * `mle::window::Key` enum values. It is case-insensitive and supports symbolic names for both
 * keyboard and mouse buttons.
 *
 * @param str The UTF-8 string to convert, such as `"a"`, `"F1"`, `"escape"`, or `"rmb"`.
 * @return An `Expected<Key>` containing the key if recognized, or an error if the string is invalid.
 *
 * @see Key for the full list of supported key codes.
 */
Expected<Key> toKey(const std::string& str);

/// Converts a character code (p, r, u, d) to a KeyState.
KeyState toKeyState(char c);

/**
 * @brief Parses a modifier flag string into a KeyModFlags bitmask.
 *
 * This function interprets a compact string representation of modifier states
 * and returns the corresponding `KeyModFlags` bitmask.
 *
 * The input string can represent the state of Shift, Control, and Alt keys using:
 * - `'1'` or `'y'` → required (must be pressed)
 * - `'0'` or `'n'` → forbidden (must not be pressed)
 * - `'*'`          → wildcard (ignored; can be either pressed or not)
 *
 * The string can be:
 * - A **single character** modifier:
 *   - `"s"` → Shift required
 *   - `"c"` → Control required
 *   - `"a"` → Alt required
 *   - `"*"` → All modifiers are wildcard (any state)
 *
 * - A **three-character string** for full control over each modifier:
 *   - `"1n*"` → Shift required, Control forbidden, Alt wildcard
 *   - `"***"` → All modifiers are wildcard
 *   - `"yn0"` → Shift required, Control required, Alt forbidden
 *
 * @param str A compact string describing the desired modifier states.
 * @return A `KeyModFlags` bitmask to match against input states.
 *
 * @note If the string is empty or invalid, returns `KeyModFlagBits::NONE`.
 *
 * @see makeKeyModFlags
 *
 * @code
 * toKeyModFlags("s");    // SHIFT only
 * toKeyModFlags("c");    // CTRL only
 * toKeyModFlags("*");    // SHIFT_ANY | CTRL_ANY | ALT_ANY
 * toKeyModFlags("1n0");  // SHIFT | CTRL_NO | ALT_NO
 * toKeyModFlags("**1");  // SHIFT_ANY | CTRL_ANY | ALT
 * @endcode
 */
KeyModFlags toKeyModFlags(const std::string& str);

/// States used for constructing modifier flags.
enum class TargetState : i8 { Y, N, ANY };

/// Constructs a KeyModFlags mask from modifier states.
KeyModFlags makeKeyModFlags(TargetState shift, TargetState ctrl, TargetState alt);
}  // namespace mle::window
