#pragma once

#include "mle/math/Types.h"
#include "mle/utils/EventDispatcher.h"

namespace mle::window::ev {
struct Close {};
struct Resize {
    vec2i size{0};
};
struct Iconify {
    bool iconified;
};
struct Focus {
    bool focus;
};

/// Dispatcher type for window-related events.
using Dispatcher = EventDispatcherST<Close, Resize, Iconify, Focus>;

/// Listener handle for `WindowClose` events.
using CloseL = Dispatcher::ListenerHnd<Close>;
/// Listener handle for `WindowResize` events.
using ResizeL = Dispatcher::ListenerHnd<Resize>;
/// Listener handle for `WindowIconify` events.
using IconifyL = Dispatcher::ListenerHnd<Iconify>;
/// Listener handle for `WindowFocus` events.
using FocusL = Dispatcher::ListenerHnd<Focus>;
}  // namespace mle::window::ev
