#pragma once

#include "mle/window/Types.h"

namespace mle {
Key systemKeyToKey(int code);

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
KeyModFlags makeKeyModFlags(TargetKeyModState shift, TargetKeyModState ctrl, TargetKeyModState alt);

}  // namespace mle
