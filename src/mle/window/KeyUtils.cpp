#include "KeyUtils.h"

#include <algorithm>

#include "SDL3/SDL_scancode.h"
#include "mle/utils/String.h"

namespace mle {
// clang-format off
// NOLINTNEXTLINE
#define SDL_KEY_MAP(X)                            \
    X(A, A) X(B, B) X(C, C) X(D, D) X(E, E)       \
    X(F, F) X(G, G) X(H, H) X(I, I) X(J, J)       \
    X(K, K) X(L, L) X(M, M) X(N, N) X(O, O)       \
    X(P, P) X(Q, Q) X(R, R) X(S, S) X(T, T)       \
    X(U, U) X(V, V) X(W, W) X(X, X) X(Y, Y) X(Z, Z) \
    X(0, ZERO) X(1, ONE) X(2, TWO) X(3, THREE)    \
    X(4, FOUR) X(5, FIVE) X(6, SIX) X(7, SEVEN)   \
    X(8, EIGHT) X(9, NINE)                        \
    X(F1, F1) X(F2, F2) X(F3, F3) X(F4, F4)       \
    X(F5, F5) X(F6, F6) X(F7, F7) X(F8, F8)       \
    X(F9, F9) X(F10, F10) X(F11, F11) X(F12, F12) \
    X(RIGHT, RIGHT) X(LEFT, LEFT) X(DOWN, DOWN) X(UP, UP) \
    X(LSHIFT, LEFT_SHIFT) X(LCTRL, LEFT_CONTROL)  \
    X(LALT, LEFT_ALT)     X(LGUI, LEFT_SUPER)     \
    X(RSHIFT, RIGHT_SHIFT) X(RCTRL, RIGHT_CONTROL) \
    X(RALT, RIGHT_ALT)     X(RGUI, RIGHT_SUPER)   \
    X(ESCAPE, ESCAPE) X(SPACE, SPACE)             \
    X(RETURN, ENTER) X(TAB, TAB) X(BACKSPACE, BACKSPACE) \
    X(INSERT, INSERT) X(DELETE, DELETE)   \
    X(PAGEUP, PAGE_UP) X(PAGEDOWN, PAGE_DOWN)     \
    X(HOME, HOME) X(END, END)                     \
    X(CAPSLOCK, CAPS_LOCK) X(SCROLLLOCK, SCROLL_LOCK) \
    X(NUMLOCKCLEAR, NUM_LOCK) X(PRINTSCREEN, PRINT_SCREEN) \
    X(PAUSE, PAUSE) X(APPLICATION, MENU)          \
    X(GRAVE, GRAVE_ACCENT) X(MINUS, MINUS)        \
    X(EQUALS, EQUAL)                              \
    X(LEFTBRACKET, LEFT_BRACKET) X(RIGHTBRACKET, RIGHT_BRACKET) \
    X(SEMICOLON, SEMICOLON) X(APOSTROPHE, APOSTROPHE) \
    X(COMMA, COMMA) X(PERIOD, PERIOD) X(SLASH, SLASH) X(BACKSLASH, BACKSLASH)
// clang-format on

namespace {
constexpr std::array<Key, SDL_SCANCODE_COUNT> buildSdlToKey() {
    std::array<Key, SDL_SCANCODE_COUNT> m{};
    for (auto& k : m) {
        k = Key::UNKNOWN;
    }

#define X(SC, K) m[SDL_SCANCODE_##SC] = Key::K;
    SDL_KEY_MAP(X)
#undef X

    return m;
}

constexpr auto SDL_TO_KEY = buildSdlToKey();
}  // namespace

Key systemKeyToKey(int code) {
    if (code < 0 || code >= SDL_SCANCODE_COUNT) {
        return Key::UNKNOWN;
    }
    return SDL_TO_KEY.at(code);
}

Expected<Key> toKey(std::string str) {
    // clang-format off
    static const std::unordered_map<std::string, Key> TABLE = {
        // Main names
        {"a", Key::A}, {"b", Key::B}, {"c", Key::C}, {"d", Key::D}, {"e", Key::E}, {"f", Key::F}, {"g", Key::G},
        {"h", Key::H}, {"i", Key::I}, {"j", Key::J}, {"k", Key::K}, {"l", Key::L}, {"m", Key::M}, {"n", Key::N},
        {"o", Key::O}, {"p", Key::P}, {"q", Key::Q}, {"r", Key::R}, {"s", Key::S}, {"t", Key::T}, {"u", Key::U},
        {"v", Key::V}, {"w", Key::W}, {"x", Key::X}, {"y", Key::Y}, {"z", Key::Z},
        {"1", Key::ONE}, {"2", Key::TWO}, {"3", Key::THREE}, {"4", Key::FOUR}, {"5", Key::FIVE},
        {"6", Key::SIX}, {"7", Key::SEVEN}, {"8", Key::EIGHT}, {"9", Key::NINE}, {"0", Key::ZERO},
        {"f1", Key::F1}, {"f2", Key::F2}, {"f3", Key::F3}, {"f4", Key::F4}, {"f5", Key::F5}, {"f6", Key::F6},
        {"f7", Key::F7}, {"f8", Key::F8}, {"f9", Key::F9}, {"f10", Key::F10}, {"f11", Key::F11}, {"f12", Key::F12},
        {"right", Key::RIGHT}, {"left", Key::LEFT}, {"down", Key::DOWN}, {"up", Key::UP},
        {"shift", Key::LEFT_SHIFT}, {"ctrl", Key::LEFT_CONTROL}, {"alt", Key::LEFT_ALT}, {"super", Key::LEFT_SUPER},
        {"rshift", Key::RIGHT_SHIFT}, {"rctrl", Key::RIGHT_CONTROL}, {"ralt", Key::RIGHT_ALT}, {"rsuper", Key::RIGHT_SUPER},
        {"escape", Key::ESCAPE}, {"space", Key::SPACE}, {"enter", Key::ENTER}, {"tab", Key::TAB}, {"backspace", Key::BACKSPACE},
        {"insert", Key::INSERT}, {"delete", Key::DELETE}, {"pageup", Key::PAGE_UP}, {"pagedown", Key::PAGE_DOWN},
        {"home", Key::HOME}, {"end", Key::END}, {"capslock", Key::CAPS_LOCK}, {"scrolllock", Key::SCROLL_LOCK},
        {"numlock", Key::NUM_LOCK}, {"printscreen", Key::PRINT_SCREEN}, {"pause", Key::PAUSE}, {"menu", Key::MENU},
        {"grave", Key::GRAVE_ACCENT}, {"minus", Key::MINUS}, {"equal", Key::EQUAL}, {"lbracket", Key::LEFT_BRACKET},
        {"rbracket", Key::RIGHT_BRACKET}, {"semicolon", Key::SEMICOLON}, {"apostrophe", Key::APOSTROPHE},
        {"comma", Key::COMMA}, {"period", Key::PERIOD}, {"slash", Key::SLASH}, {"backslash", Key::BACKSLASH},
        {"mouse_left", Key::MOUSE_LEFT}, {"mouse_right", Key::MOUSE_RIGHT}, {"mouse_middle", Key::MOUSE_MIDDLE},
        {"mouse_four", Key::MOUSE_FOUR}, {"mouse_five", Key::MOUSE_FIVE}, {"mouse_six", Key::MOUSE_SIX},
        {"mouse_seven", Key::MOUSE_SEVEN}, {"mouse_eight", Key::MOUSE_EIGHT},

        // Aliases
        {"esc", Key::ESCAPE}, {"return", Key::ENTER}, {"del", Key::DELETE}, {"ins", Key::INSERT},
        {"bksp", Key::BACKSPACE}, {"win", Key::LEFT_SUPER}, {"cmd", Key::LEFT_SUPER}, {"option", Key::LEFT_ALT},
        {"lalt", Key::LEFT_ALT}, {"ralt", Key::RIGHT_ALT}, {"lwin", Key::LEFT_SUPER}, {"rwin", Key::RIGHT_SUPER},
        {"pgup", Key::PAGE_UP}, {"pgdn", Key::PAGE_DOWN}, {"caps", Key::CAPS_LOCK}, {"scroll", Key::SCROLL_LOCK},
        {"num", Key::NUM_LOCK}, {"prtsc", Key::PRINT_SCREEN}, {"menu", Key::MENU}, {"spacebar", Key::SPACE},
        {"enterkey", Key::ENTER}, {"back", Key::BACKSPACE}, {"deletekey", Key::DELETE}, {"tabkey", Key::TAB},
        {"homekey", Key::HOME}, {"endkey", Key::END}, {"pausebreak", Key::PAUSE}, {"print", Key::PRINT_SCREEN},
        {"apps", Key::MENU}, {"graveaccent", Key::GRAVE_ACCENT}, {"dash", Key::MINUS}, {"equalsign", Key::EQUAL},
        {"openbracket", Key::LEFT_BRACKET}, {"closebracket", Key::RIGHT_BRACKET}, {"semi", Key::SEMICOLON},
        {"quote", Key::APOSTROPHE}, {"comma_key", Key::COMMA}, {"dot", Key::PERIOD}, {"period_key", Key::PERIOD},
        {"forwardslash", Key::SLASH}, {"backslash_key", Key::BACKSLASH},
        {"mouse1", Key::MOUSE_LEFT}, {"mouse2", Key::MOUSE_RIGHT}, {"mouse3", Key::MOUSE_MIDDLE},
        {"mouse4", Key::MOUSE_FOUR}, {"mouse5", Key::MOUSE_FIVE}, {"mouse6", Key::MOUSE_SIX},
        {"mouse7", Key::MOUSE_SEVEN}, {"mouse8", Key::MOUSE_EIGHT}
    };
    // clang-format on

    std::ranges::transform(str, str.begin(), ::tolower);

    auto it = TABLE.find(str);
    if (it != TABLE.end()) {
        return it->second;
    }

    MLE_W("Unknown key: {}", str);
    return std::unexpected(Result::INVALID_ARGUMENT);
}

Expected<KeyState> toKeyState(char c) {
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
            MLE_W("Unknown char: {}", c);
            return std::unexpected(Result::INVALID_ARGUMENT);
    }
}

Expected<Keybinding> toKeybinding(std::string_view str) {
    const auto parts = split(str, ':', false);
    if (parts.empty()) {
        MLE_W("Empty keybinding string");
        return std::unexpected(Result::INVALID_ARGUMENT);
    }

    Keybinding ret{};

    std::string key_str(parts[0]);
    auto key_result = toKey(key_str);
    if (!key_result) {
        MLE_W("Invalid key: {}", parts[0]);
        return std::unexpected(key_result.error());
    }
    ret.key = *key_result;

    if (parts.size() >= 2) {
        char c = parts[1][0];
        auto key_state_result = toKeyState(c);
        if (!key_state_result) {
            MLE_W("Invalid key state: {}", c);
            return std::unexpected(key_state_result.error());
        }
        ret.state = *key_state_result;
    }

    if (parts.size() == 3) {
        ret.mods = toKeyModFlags(parts[2]);
    }

    return ret;
}

KeyModFlags toKeyModFlags(std::string_view str) {
    if (str.empty()) {
        return KeyModFlagBits::NONE;
    }

    std::array<TargetKeyModState, 3> states = {Keybinding::DEFAULT_TARGET_STATE, Keybinding::DEFAULT_TARGET_STATE, Keybinding::DEFAULT_TARGET_STATE};

    if (str.size() == 1) {
        if (str[0] == 's') {
            states[0] = TargetKeyModState::Y;
        } else if (str[0] == 'c') {
            states[1] = TargetKeyModState::Y;
        } else if (str[0] == 'a') {
            states[2] = TargetKeyModState::Y;
        } else if (str[0] == '*') {
            states[0] = TargetKeyModState::ANY;
            states[1] = TargetKeyModState::ANY;
            states[2] = TargetKeyModState::ANY;
        }
    } else if (str.size() == 3) {
        for (i8 i = 0; i < 3; ++i) {
            if (str[i] == '1' || str[i] == 'y') {
                states.at(i) = TargetKeyModState::Y;
            } else if (str[i] == '0' || str[i] == 'n') {
                states.at(i) = TargetKeyModState::N;
            } else if (str[i] == '*') {
                states.at(i) = TargetKeyModState::ANY;
            }
        }
    }
    return makeKeyModFlags(states[0], states[1], states[2]);
}

KeyModFlags makeKeyModFlags(TargetKeyModState shift, TargetKeyModState ctrl, TargetKeyModState alt) {
    KeyModFlags flags{};
    if (shift == TargetKeyModState::Y) {
        flags |= KeyModFlagBits::SHIFT;
    } else if (shift == TargetKeyModState::ANY) {
        flags |= KeyModFlagBits::SHIFT_ANY;
    } else {
        flags |= KeyModFlagBits::SHIFT_NO;
    }
    if (ctrl == TargetKeyModState::Y) {
        flags |= KeyModFlagBits::CTRL;
    } else if (ctrl == TargetKeyModState::ANY) {
        flags |= KeyModFlagBits::CTRL_ANY;
    } else {
        flags |= KeyModFlagBits::CTRL_NO;
    }
    if (alt == TargetKeyModState::Y) {
        flags |= KeyModFlagBits::ALT;
    } else if (alt == TargetKeyModState::ANY) {
        flags |= KeyModFlagBits::ALT_ANY;
    } else {
        flags |= KeyModFlagBits::ALT_NO;
    }
    return flags;
}

}  // namespace mle
