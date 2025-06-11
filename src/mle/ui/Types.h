#pragma once

#include <entt/entt.hpp>

#include "mle/common/Types.h"
#include "mle/common/Utils.h"

namespace mle::ui {}  // namespace mle::ui

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
