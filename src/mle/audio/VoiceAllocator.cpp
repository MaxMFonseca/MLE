#include "VoiceAllocator.h"

namespace mle::audio {
bool VoiceSelection::accepted() const {
    return index != max<usize>();
}

VoiceSelection selectVoice(std::span<const VoiceMetadata> voices, std::span<const BusVoicePolicy, BUS_COUNT> policies, u8 incoming_bus, u32 incoming_priority) {
    if (incoming_bus >= BUS_COUNT) {
        return {.reason = VoiceRejectReason::INVALID_BUS};
    }
    if (incoming_priority == 0) {
        return {.reason = VoiceRejectReason::ZERO_PRIORITY};
    }

    const auto& incoming_policy = policies[incoming_bus];
    usize bus_voice_count = 0;
    for (const auto& voice : voices) {
        if (voice.priority != 0 && voice.bus == incoming_bus) {
            ++bus_voice_count;
        }
    }

    if (incoming_policy.max_voices != 0 && bus_voice_count >= incoming_policy.max_voices) {
        auto victim = max<usize>();
        auto victim_priority = max<u32>();
        for (usize i = 0; i < voices.size(); ++i) {
            const auto& voice = voices[i];
            if (voice.priority != 0 && voice.bus == incoming_bus && voice.priority < victim_priority) {
                victim = i;
                victim_priority = voice.priority;
            }
        }
        if (victim == max<usize>()) {
            return {.reason = VoiceRejectReason::BUS_CAP};
        }
        if (incoming_priority <= victim_priority) {
            return {.reason = VoiceRejectReason::INSUFFICIENT_PRIORITY};
        }
        return {.index = victim};
    }

    for (usize i = 0; i < voices.size(); ++i) {
        if (voices[i].priority == 0) {
            return {.index = i};
        }
    }

    auto victim = max<usize>();
    auto victim_priority = max<u32>();
    for (usize i = 0; i < voices.size(); ++i) {
        const auto& voice = voices[i];
        if (voice.bus >= BUS_COUNT) {
            continue;
        }
        if (voice.bus != incoming_bus && policies[voice.bus].protected_from_other_buses) {
            continue;
        }
        if (voice.priority < victim_priority) {
            victim = i;
            victim_priority = voice.priority;
        }
    }

    if (victim == max<usize>()) {
        return {.reason = VoiceRejectReason::NO_ELIGIBLE_VICTIM};
    }
    if (incoming_priority <= victim_priority) {
        return {.reason = VoiceRejectReason::INSUFFICIENT_PRIORITY};
    }
    return {.index = victim};
}

void resetVoice(VoiceMetadata& voice) {
    voice = {};
}
}  // namespace mle::audio
