#pragma once

#include <functional>
#include <utility>

namespace mle::client {
template <typename GameShutdown, typename DebugShutdown, typename AudioShutdown>
void shutdownLayersBeforeAudio(GameShutdown&& shutdown_game, DebugShutdown&& shutdown_debug, AudioShutdown&& shutdown_audio) {
    std::invoke(std::forward<GameShutdown>(shutdown_game));
    std::invoke(std::forward<DebugShutdown>(shutdown_debug));
    std::invoke(std::forward<AudioShutdown>(shutdown_audio));
}
}  // namespace mle::client
