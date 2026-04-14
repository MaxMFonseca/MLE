#pragma once

#include <entt/entt.hpp>

#include "mle/math/Types.h"
#include "mle/utils/Utils.h"
#include "spdlog/fmt/bundled/base.h"

namespace fmt {
template <>
struct formatter<entt::entity> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(entt::entity v, FormatContext& ctx) const {
        if (v == entt::null) {
            return format_to(ctx.out(), "<null>");
        }
        return format_to(ctx.out(), "{}", entt::to_integral(v));
    }
};
}  // namespace fmt
