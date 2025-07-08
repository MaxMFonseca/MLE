#pragma once

#include <entt/entt.hpp>

#include "mle/common/Utils.h"

namespace fmt {
using namespace mle;  // NOLINT

template <>
struct formatter<entt::entity> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(entt::entity v, FormatContext& ctx) const {
        return format_to(ctx.out(), "{}", as<u32>(v));
    }
};
}  // namespace fmt
