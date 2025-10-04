
#include "Utils.h"

#include "mle/common/Logger.h"
#include "mle/common/Utils.h"
#include "mle/window/Types.h"

namespace mle::window {
Key castGlfwKeyToKey(i32 code) {
    // clang-format off
#define CASE(x) case GLFW_KEY_##x: return Key::x; // NOLINT
    switch (code) {
        CASE(A) CASE(B) CASE(C) CASE(D) CASE(E) CASE(F) CASE(G) CASE(H) CASE(I) CASE(J) CASE(K) CASE(L) CASE(M)
        CASE(N) CASE(O) CASE(P) CASE(Q) CASE(R) CASE(S) CASE(T) CASE(U) CASE(V) CASE(W) CASE(X) CASE(Y) CASE(Z)
        CASE(F1) CASE(F2) CASE(F3) CASE(F4) CASE(F5) CASE(F6) CASE(F7) CASE(F8) CASE(F9) CASE(F10) CASE(F11) CASE(F12) 
        CASE(RIGHT) CASE(LEFT) CASE(DOWN) CASE(UP) 
        CASE(LEFT_SHIFT) CASE(LEFT_CONTROL) CASE(LEFT_ALT) CASE(LEFT_SUPER) 
        CASE(RIGHT_SHIFT) CASE(RIGHT_CONTROL) CASE(RIGHT_ALT) CASE(RIGHT_SUPER) 
        CASE(ESCAPE) CASE(SPACE) CASE(ENTER) CASE(TAB) CASE(BACKSPACE) CASE(INSERT) CASE(DELETE) CASE(PAGE_UP) CASE(PAGE_DOWN) 
        CASE(HOME) CASE(END) CASE(CAPS_LOCK) CASE(SCROLL_LOCK) CASE(NUM_LOCK) CASE(PRINT_SCREEN) CASE(PAUSE) CASE(MENU)
        CASE(GRAVE_ACCENT) CASE(MINUS) CASE(EQUAL) CASE(LEFT_BRACKET) CASE(RIGHT_BRACKET) CASE(SEMICOLON) CASE(APOSTROPHE)
        case GLFW_KEY_0: return Key::ZERO;
        case GLFW_KEY_1: return Key::ONE;
        case GLFW_KEY_2: return Key::TWO;
        case GLFW_KEY_3: return Key::THREE;
        case GLFW_KEY_4: return Key::FOUR;
        case GLFW_KEY_5: return Key::FIVE;
        case GLFW_KEY_6: return Key::SIX;
        case GLFW_KEY_7: return Key::SEVEN;
        case GLFW_KEY_8: return Key::EIGHT;
        case GLFW_KEY_9: return Key::NINE;
        default: {
            MLE_W("Unknown key: {}", code);
            return Key::UNKNOWN;
        }
    }
#undef CASE
    // clang-format on
}

Key castGlfwMouseButtonToKey(i32 code) {
    return Key(static_cast<i32>(Key::MOUSE_ONE) + code);
}

Expected<std::tuple<Key, KeyState, KeyModFlags>> toKeyTuple(const std::string& str) {
    const auto parts = split(str, ':');
    if (parts.empty()) {
        MLE_W("Empty key tuple string");
        return std::unexpected(Result::INVALID_ARGUMENT);
    }

    auto key_result = toKey(parts[0]);
    if (!key_result) {
        MLE_W("Invalid key: {}", parts[0]);
        return std::unexpected(key_result.error());
    }

    KeyState state = KeyState::PRESSED;
    if (parts.size() >= 2) {
        state = toKeyState(parts[1][0]);
    }

    KeyModFlags mods = KeyModFlagBits::NONE;
    if (parts.size() == 3) {
        mods = toKeyModFlags(parts[2]);
    }

    return std::make_tuple(*key_result, state, mods);
}

Expected<Key> toKey(const std::string& s) {
    // clang-format off
#define LOOKUP_ENTRY(a, A) {#a, Key::A}, // NOLINT
    static std::unordered_map<std::string, Key> key_name_lookup{
    LOOKUP_ENTRY(a, A) LOOKUP_ENTRY(b, B) LOOKUP_ENTRY(c, C) LOOKUP_ENTRY(d, D) LOOKUP_ENTRY(e, E) LOOKUP_ENTRY(f, F)
    LOOKUP_ENTRY(g, G) LOOKUP_ENTRY(h, H) LOOKUP_ENTRY(i, I) LOOKUP_ENTRY(j, J) LOOKUP_ENTRY(k, K) LOOKUP_ENTRY(l, L)
    LOOKUP_ENTRY(m, M) LOOKUP_ENTRY(n, N) LOOKUP_ENTRY(o, O) LOOKUP_ENTRY(p, P) LOOKUP_ENTRY(q, Q) LOOKUP_ENTRY(r, R)
    LOOKUP_ENTRY(s, S) LOOKUP_ENTRY(t, T) LOOKUP_ENTRY(u, U) LOOKUP_ENTRY(v, V) LOOKUP_ENTRY(w, W) LOOKUP_ENTRY(x, X)
    LOOKUP_ENTRY(y, Y) LOOKUP_ENTRY(z, Z) LOOKUP_ENTRY(0, ZERO) LOOKUP_ENTRY(1, ONE) LOOKUP_ENTRY(2, TWO) LOOKUP_ENTRY(3, THREE)
    LOOKUP_ENTRY(4, FOUR) LOOKUP_ENTRY(5, FIVE) LOOKUP_ENTRY(6, SIX) LOOKUP_ENTRY(7, SEVEN) LOOKUP_ENTRY(8, EIGHT) LOOKUP_ENTRY(9, NINE)
    LOOKUP_ENTRY(f1, F1) LOOKUP_ENTRY(f2, F2) LOOKUP_ENTRY(f3, F3) LOOKUP_ENTRY(f4, F4) LOOKUP_ENTRY(f5, F5) LOOKUP_ENTRY(f6, F6)
    LOOKUP_ENTRY(f7, F7) LOOKUP_ENTRY(f8, F8) LOOKUP_ENTRY(f9, F9) LOOKUP_ENTRY(f10, F10) LOOKUP_ENTRY(f11, F11) LOOKUP_ENTRY(f12, F12)
    LOOKUP_ENTRY(F1, F1) LOOKUP_ENTRY(F2, F2) LOOKUP_ENTRY(F3, F3) LOOKUP_ENTRY(F4, F4) LOOKUP_ENTRY(F5, F5) LOOKUP_ENTRY(F6, F6)
    LOOKUP_ENTRY(F7, F7) LOOKUP_ENTRY(F8, F8) LOOKUP_ENTRY(F9, F9) LOOKUP_ENTRY(F10, F10) LOOKUP_ENTRY(F11, F11) LOOKUP_ENTRY(F12, F12)
    LOOKUP_ENTRY(right, RIGHT) LOOKUP_ENTRY(left, LEFT) LOOKUP_ENTRY(down, DOWN) LOOKUP_ENTRY(up, UP)
    LOOKUP_ENTRY(left_shift, LEFT_SHIFT) LOOKUP_ENTRY(left_control, LEFT_CONTROL) LOOKUP_ENTRY(left_alt, LEFT_ALT) LOOKUP_ENTRY(left_super, LEFT_SUPER)
    LOOKUP_ENTRY(right_shift, RIGHT_SHIFT) LOOKUP_ENTRY(right_control, RIGHT_CONTROL) LOOKUP_ENTRY(right_alt, RIGHT_ALT) LOOKUP_ENTRY(right_super,
    RIGHT_SUPER) LOOKUP_ENTRY(escape, ESCAPE) LOOKUP_ENTRY(space, SPACE) LOOKUP_ENTRY(enter, ENTER) LOOKUP_ENTRY(tab, TAB) LOOKUP_ENTRY(backspace,
    BACKSPACE) LOOKUP_ENTRY(insert, INSERT) LOOKUP_ENTRY(delete, DELETE) LOOKUP_ENTRY(page_up, PAGE_UP) LOOKUP_ENTRY(page_down, PAGE_DOWN)
    LOOKUP_ENTRY(home, HOME) LOOKUP_ENTRY(end, END) LOOKUP_ENTRY(caps_lock, CAPS_LOCK) LOOKUP_ENTRY(scroll_lock, SCROLL_LOCK)
    LOOKUP_ENTRY(num_lock, NUM_LOCK) LOOKUP_ENTRY(print_screen, PRINT_SCREEN) LOOKUP_ENTRY(pause, PAUSE) LOOKUP_ENTRY(menu, MENU)
    LOOKUP_ENTRY(grave_accent, GRAVE_ACCENT) LOOKUP_ENTRY(minus, MINUS) LOOKUP_ENTRY(equal, EQUAL) LOOKUP_ENTRY(left_bracket, LEFT_BRACKET)
    LOOKUP_ENTRY(right_bracket, RIGHT_BRACKET) LOOKUP_ENTRY(semicolon, SEMICOLON) LOOKUP_ENTRY(apostrophe, APOSTROPHE)
    LOOKUP_ENTRY(`, GRAVE_ACCENT) LOOKUP_ENTRY(-, MINUS) LOOKUP_ENTRY(=, EQUAL) LOOKUP_ENTRY([, LEFT_BRACKET) LOOKUP_ENTRY(], RIGHT_BRACKET)
    LOOKUP_ENTRY(;, SEMICOLON) {"'", Key::APOSTROPHE}, 

    LOOKUP_ENTRY(lmb, MOUSE_LEFT) LOOKUP_ENTRY(rmb, MOUSE_RIGHT) LOOKUP_ENTRY(mmb, MOUSE_MIDDLE)
    };
    // clang-format on

    if (const auto it = key_name_lookup.find(toLower(s)); it != key_name_lookup.end()) {
        return it->second;
    }
    MLE_E("Unknown key: {}", s);
    return std::unexpected(Result::INVALID_ARGUMENT);
}

KeyState toKeyState(char c) {
    switch (c) {
        case 'p':
            return KeyState::PRESSED;
        case 'r':
            return KeyState::RELEASED;
        case 'u':
            return KeyState::UP;
        case 'd':
            return KeyState::DOWN;
        default:
            MLE_UNREACHABLE_LOG("Unknown char: {}", c);
    }
}

KeyModFlags toKeyModFlags(const std::string& str) {
    if (str.empty()) {
        return KeyModFlagBits::NONE;
    }
    std::array<TargetState, 3> states = {TargetState::N, TargetState::N, TargetState::N};

    if (str.size() == 1) {
        if (str[0] == 's') {
            states[0] = TargetState::Y;
        } else if (str[0] == 'c') {
            states[1] = TargetState::Y;
        } else if (str[0] == 'a') {
            states[2] = TargetState::Y;
        } else if (str[0] == '*') {
            states[0] = TargetState::ANY;
            states[1] = TargetState::ANY;
            states[2] = TargetState::ANY;
        }
    } else if (str.size() == 3) {
        for (i8 i = 0; i < 3; ++i) {
            if (str[i] == '1' || str[i] == 'y') {
                states.at(i) = TargetState::Y;
            } else if (str[i] == '0' || str[i] == 'n') {
                states.at(i) = TargetState::N;
            } else if (str[i] == '*') {
                states.at(i) = TargetState::ANY;
            }
        }
    }
    return makeKeyModFlags(states[0], states[1], states[2]);
}

KeyModFlags makeKeyModFlags(TargetState shift, TargetState ctrl, TargetState alt) {
    KeyModFlags flags{};
    if (shift == TargetState::Y) {
        flags |= KeyModFlagBits::SHIFT;
    } else if (shift == TargetState::ANY) {
        flags |= KeyModFlagBits::SHIFT_ANY;
    } else {
        flags |= KeyModFlagBits::SHIFT_NO;
    }
    if (ctrl == TargetState::Y) {
        flags |= KeyModFlagBits::CTRL;
    } else if (ctrl == TargetState::ANY) {
        flags |= KeyModFlagBits::CTRL_ANY;
    } else {
        flags |= KeyModFlagBits::CTRL_NO;
    }
    if (alt == TargetState::Y) {
        flags |= KeyModFlagBits::ALT;
    } else if (alt == TargetState::ANY) {
        flags |= KeyModFlagBits::ALT_ANY;
    } else {
        flags |= KeyModFlagBits::ALT_NO;
    }
    return flags;
}
}  // namespace mle::window
