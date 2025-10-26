#include "String.h"

#include "mle/utils/Utils.h"

namespace mle {
std::vector<std::string_view> split(std::string_view s, char delim, bool keep_empty) {
    std::vector<std::string_view> ret;

    usize start = 0;
    while (start <= s.size()) {
        const usize pos = s.find(delim, start);
        const bool found = pos != std::string_view::npos;
        const usize end = found ? pos : s.size();

        if (keep_empty || end > start) {
            ret.emplace_back(s.substr(start, end - start));
        }
        if (!found) {
            break;
        }
        start = pos + 1;
    }
    return ret;
}

std::string toLower(std::string_view s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

std::string toUpper(std::string_view s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
        result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

std::string toUtf8(std::u32string_view s) {
    std::string result;
    result.reserve(s.size());
    utf8::unchecked::utf32to8(s.begin(), s.end(), std::back_inserter(result));
    return result;
}

std::u32string toUtf32(std::string_view s) {
    std::u32string result;
    result.reserve(s.size());
    const char* it = s.data();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    const char* end = it + s.size();
    utf8::unchecked::utf8to32(it, end, std::back_inserter(result));
    return result;
}

bool isSpace(char c) {
    return std::isspace(as<unsigned char>(c)) != 0;
}

std::string_view trim(std::string_view s) {
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    const char* begin = s.data();
    const char* end = begin + s.size();

    while (begin < end && isSpace(*begin)) {
        ++begin;
    }
    while (end > begin && isSpace(*(end - 1))) {
        --end;
    }

    return {begin, as<usize>(end - begin)};
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

std::pair<f32, std::string_view> splitNumSuffix(std::string_view s) {
    s = trim(s);

    if (s.size() > 0 && s[0] == '+') {
        s.remove_prefix(1);
    }

    const char* begin = s.data();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
    const char* end = begin + s.size();

    f32 value = 0.0F;
    const char* num_end = begin;

    auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec == std::errc()) {
        num_end = ptr;
    } else {
        return {0.0F, s};
    }

    std::string_view suffix(num_end, end - num_end);

    while (!suffix.empty() && isSpace(suffix.front())) {
        suffix.remove_prefix(1);
    }

    return {value, suffix};
}

std::string bitsToString(u64 b) {
    std::array u = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    auto v = as<f64>(b);
    while (v >= 1024.0 && i < 4) {
        v /= 1024.0;
        ++i;
    }

    if (i <= 1) {
        return fmt::format("{:.0f} {}", v, u.at(i));
    }
    return fmt::format("{:.1f} {}", v, u.at(i));
}
}  // namespace mle
