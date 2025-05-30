/**
 * @file
 * @brief Event definitions and listener types for window-related events.
 */

#pragma once

#include "mle/common/EventDispatcher.h"

namespace mle::window {
namespace events {
/// Event signaling that the window should close.
struct WindowClose {};
/// Event signaling that the window was resized.
struct WindowResize {
    vec2i size;  ///< New window size in pixels.
};
/// Event signaling that the window was iconified (minimized).
struct WindowIconify {
    bool iconified;  ///< True if the window is iconified, false if restored.
};
}  // namespace events

/// Dispatcher type for window-related events.
using ED = EventDispatcher<events::WindowClose, events::WindowResize, events::WindowIconify>;

/// Listener handle for `WindowClose` events.
using WindowCloseListener = ED::ListenerHnd<events::WindowClose>;
/// Listener handle for `WindowResize` events.
using WindowResizeListener = ED::ListenerHnd<events::WindowResize>;
/// Listener handle for `WindowIconify` events.
using WindowIconifyListener = ED::ListenerHnd<events::WindowIconify>;
}  // namespace mle::window
