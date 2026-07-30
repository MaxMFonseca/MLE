#include "StreamGroup.h"

#include <array>

namespace mle::audio {
std::string_view streamGroupRejectReasonName(StreamGroupRejectReason reason) noexcept {
    switch (reason) {
        case StreamGroupRejectReason::NONE:
            return "none";
        case StreamGroupRejectReason::EMPTY:
            return "empty";
        case StreamGroupRejectReason::TOO_MANY:
            return "too many";
        case StreamGroupRejectReason::INVALID_SLOT:
            return "invalid slot";
        case StreamGroupRejectReason::DUPLICATE_SLOT:
            return "duplicate slot";
    }
    return "unknown";
}

StreamGroupRejectReason validateStreamGroup(const cmd::StartStreamGroup& group) {
    if (group.count == 0) {
        return StreamGroupRejectReason::EMPTY;
    }
    if (group.count > STREAM_SLOT_COUNT) {
        return StreamGroupRejectReason::TOO_MANY;
    }

    std::array<bool, STREAM_SLOT_COUNT> seen{};
    for (usize i = 0; i < group.count; ++i) {
        const u8 slot = group.streams.at(i).id;
        if (slot >= STREAM_SLOT_COUNT) {
            return StreamGroupRejectReason::INVALID_SLOT;
        }
        if (seen.at(slot)) {
            return StreamGroupRejectReason::DUPLICATE_SLOT;
        }
        seen.at(slot) = true;
    }
    return StreamGroupRejectReason::NONE;
}

bool invokeSynchronizedPlay(std::span<const ALuint> sources, SourcePlayvFn playv) {
    return playv(static_cast<ALsizei>(sources.size()), sources.data());
}
}  // namespace mle::audio
