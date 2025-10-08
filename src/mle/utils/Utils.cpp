#include "Utils.h"

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
}  // namespace mle
