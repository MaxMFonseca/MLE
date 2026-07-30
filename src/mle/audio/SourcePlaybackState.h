#pragma once

#include "mle/audio/Types.h"

namespace mle::audio {
struct SourcePlaybackState {
    bool relative = true;
    vec3f position{};
    vec3f velocity{};
};

[[nodiscard]] SourcePlaybackState sourcePlaybackState(const PlayParams& params);
}  // namespace mle::audio
