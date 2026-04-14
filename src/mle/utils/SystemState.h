#pragma once

#include "mle/math/Types.h"

namespace mle {
enum class SystemState : u8 {
    UNINITIALIZED,
    INITIALIZED,
    RUNNING,
    STOPPING,
};
}  // namespace mle

namespace fmt {
template <>
struct formatter<mle::SystemState> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::SystemState& state, FormatContext& ctx) const {
        std::string state_str;
        switch (state) {
            case mle::SystemState::UNINITIALIZED:
                state_str = "UNINITIALIZED";
                break;
            case mle::SystemState::INITIALIZED:
                state_str = "INITIALIZED";
                break;
            case mle::SystemState::RUNNING:
                state_str = "RUNNING";
                break;
            case mle::SystemState::STOPPING:
                state_str = "STOPPING";
                break;
        }
        return format_to(ctx.out(), "{}", state_str);
    }
};
}  // namespace fmt
