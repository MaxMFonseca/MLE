#include "SourcePlaybackState.h"

namespace mle::audio {
SourcePlaybackState sourcePlaybackState(const PlayParams& params) {
    if (!params.spatial) {
        return {};
    }

    return {
        .relative = false,
        .position = params.position,
        .velocity = params.velocity,
    };
}
}  // namespace mle::audio
