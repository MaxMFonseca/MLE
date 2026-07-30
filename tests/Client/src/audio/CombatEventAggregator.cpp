#include "CombatEventAggregator.h"

namespace mle::audio {
namespace {
constexpr std::array KINDS{
    CombatCueKind::CRITICAL,
    CombatCueKind::HEAVY,
    CombatCueKind::IMPACT,
    CombatCueKind::CROWD,
};
}  // namespace

CombatFrameResult CombatEventAggregator::advance(f32 dt_seconds, u32 accepted_events) noexcept {
    static_cast<void>(dt_seconds);

    CombatFrameResult result{};
    result.raw = accepted_events;

    std::array<u32, KINDS.size()> bucket_counts{};
    const u32 events_per_bucket = accepted_events / static_cast<u32>(KINDS.size());
    bucket_counts.fill(events_per_bucket);

    const u32 remainder = accepted_events % static_cast<u32>(KINDS.size());
    for (u32 i = 0; i < remainder; ++i) {
        ++bucket_counts.at((next_kind_ + i) % KINDS.size());
    }
    next_kind_ = (next_kind_ + remainder) % KINDS.size();

    for (u32 bucket = 0; bucket < KINDS.size(); ++bucket) {
        if (bucket_counts.at(bucket) == 0) {
            continue;
        }

        result.cues.at(result.submitted++) = CombatCue{
            .kind = KINDS.at(bucket),
            .priority = CombatCue::MAX_PRIORITY - bucket,
            .event_count = bucket_counts.at(bucket),
        };
        result.aggregated += bucket_counts.at(bucket);
    }

    result.dropped = result.raw - result.aggregated;
    return result;
}
}  // namespace mle::audio
