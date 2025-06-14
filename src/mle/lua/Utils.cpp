#include "Utils.h"

namespace mle::lua {
std::optional<sol::object> getKeyOrIdx(const sol::table& table, const std::string& key, int idx) {
    auto iret = table[idx];
    if (iret.valid()) {
        return iret;
    }
    auto kret = table[key];
    if (kret.valid()) {
        return kret;
    }
    return std::nullopt;
}

}  // namespace mle::lua
