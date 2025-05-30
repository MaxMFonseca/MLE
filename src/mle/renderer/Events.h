#pragma once

#include "Types.h"
#include "mle/common/EventDispatcher.h"

namespace mle::renderer {
namespace events {
struct SwapchainRecreated {
    vk::Format old_format{};
    vk::Format new_format{};
};
}  // namespace events

using namespace events;
using ED = EventDispatcher<SwapchainRecreated>;

using SwapchainRecreatedListener = ED::ListenerHnd<SwapchainRecreated>;
}  // namespace mle::renderer
