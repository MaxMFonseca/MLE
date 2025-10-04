#pragma once

#include "mle/common/EventDispatcher.h"

namespace mle::ui {
namespace events {
struct RootCreated {};
}  // namespace events

using ED = EventDispatcher<events::RootCreated>;

using RootCreatedListener = ED::ListenerHnd<events::RootCreated>;

}  // namespace mle::ui
