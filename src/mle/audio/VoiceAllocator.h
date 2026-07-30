#pragma once

#include <span>

#include "mle/utils/Types.h"

namespace mle::audio {
inline constexpr usize BUS_COUNT = 8;

struct BusVoicePolicy {
    u16 max_voices = 0;
    bool protected_from_other_buses = false;
};

struct VoiceMetadata {
    u32 priority = 0;
    u8 bus = 0;
    f32 volume = 1.0F;
};

enum class VoiceRejectReason : u8 {
    NONE,
    INVALID_BUS,
    ZERO_PRIORITY,
    BUS_CAP,
    INSUFFICIENT_PRIORITY,
    NO_ELIGIBLE_VICTIM,
};

struct VoiceSelection {
    usize index = max<usize>();
    VoiceRejectReason reason = VoiceRejectReason::NONE;

    [[nodiscard]] bool accepted() const;
};

VoiceSelection selectVoice(std::span<const VoiceMetadata> voices, std::span<const BusVoicePolicy, BUS_COUNT> policies, u8 incoming_bus, u32 incoming_priority);
void resetVoice(VoiceMetadata& voice);
}  // namespace mle::audio
