#include "Utils.h"

#include "mle/lua/Lua.h"

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

void merge(sol::table& dst, const sol::table& src) {  // NOLINT
    for (const auto& [key, val] : src) {
        if (val.get_type() == sol::type::table) {
            auto dst_val = dst[key];
            if (!dst_val.valid() || !dst_val.is<sol::table>()) {
                dst[key] = createTable(val, true);
            } else {
                auto dst_val_table = dst_val.get<sol::table>();
                merge(dst_val_table, val.as<sol::table>());
            }
        } else {
            dst[key] = val;
        }
    }
}
}  // namespace mle::lua
