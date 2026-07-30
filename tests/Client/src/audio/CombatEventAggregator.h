#pragma once

#include <array>

#include "mle/math/Types.h"

namespace mle::audio {
enum class CombatCueKind : u8 {
    CRITICAL,
    HEAVY,
    IMPACT,
    CROWD,
};

struct CombatCue {
    static constexpr u32 MAX_PRIORITY = 4;

    CombatCueKind kind{CombatCueKind::IMPACT};
    u32 priority{};
    u32 event_count{};
};

struct CombatFrameResult {
    std::array<CombatCue, 4> cues{};
    u32 raw{};
    u32 aggregated{};
    u32 submitted{};
    u32 dropped{};
};

/**
 * Deterministically maps accepted combat events onto four semantic cue kinds.
 * Multiple events of one kind are coalesced into one cue descriptor per frame.
 *
 * Counter invariant: raw counts accepted input events; submitted counts emitted
 * cue descriptors (never events); aggregated counts raw events represented by
 * those descriptors; dropped counts unrepresented raw events. Consequently,
 * every result satisfies raw == aggregated + dropped. submitted is not part of
 * the event balance.
 */
class CombatEventAggregator {
  public:
    static constexpr u32 MAX_PENDING_EVENTS = 0;

    [[nodiscard]] CombatFrameResult advance(f32 dt_seconds, u32 accepted_events) noexcept;
    [[nodiscard]] static constexpr u32 pendingEvents() noexcept { return 0; }

  private:
    u32 next_kind_{};
};
}  // namespace mle::audio
