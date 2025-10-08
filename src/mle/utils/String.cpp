#include "String.h"

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
}  // namespace mle
