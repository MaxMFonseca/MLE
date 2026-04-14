#pragma once

#include <SDL3/SDL_keycode.h>

#include "mle/window/Types.h"

namespace mle {
Key systemKeyToKey(SDL_Keycode kc);

constexpr bool isPrintableKey(Key k) {
    return k <= Key::LAST_PRINTABLE_CHAR;
}

constexpr bool isMouseKey(Key k) {
    return k >= Key::MOUSE_ONE && k <= Key::MOUSE_EIGHT;
}

inline Key systemMouseButtonToKey(int code) {
    return Key(as<i32>(Key::MOUSE_ONE) + code - 1);
}

inline u32 packKeyKeyState(Key key, KeyState state) {
    return static_cast<u32>(key) | (static_cast<u32>(state) << 8U);
}

Expected<Keybinding> toKeybinding(std::string_view str);

Expected<Key> toKey(std::string str);

Expected<KeyState> toKeyState(char c);

KeyModFlags toKeyModFlags(std::string_view str);

}  // namespace mle
