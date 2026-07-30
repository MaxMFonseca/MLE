#pragma once

#include <span>
#include <string_view>

#include "mle/audio/Types.h"

namespace mle::audio {
enum class StreamGroupRejectReason : u8 {
    NONE,
    EMPTY,
    TOO_MANY,
    INVALID_SLOT,
    DUPLICATE_SLOT,
};

[[nodiscard]] std::string_view streamGroupRejectReasonName(StreamGroupRejectReason reason) noexcept;
[[nodiscard]] StreamGroupRejectReason validateStreamGroup(const cmd::StartStreamGroup& group);

using SourcePlayvFn = bool (*)(ALsizei count, const ALuint* sources);
[[nodiscard]] bool invokeSynchronizedPlay(std::span<const ALuint> sources, SourcePlayvFn playv);
}  // namespace mle::audio
