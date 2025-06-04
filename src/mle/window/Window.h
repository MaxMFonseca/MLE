/**
 * @file
 * @brief Window subsystem interface.
 *
 * Uses GLFW unser the hood to create and manage a window for the engine.
 * Provides functions for initializing, updating, and shutting down the window.
 * Also exposes window configuration, input handling, and window event dispatching.
 * No multiple windows are supported for now.
 */

#pragma once

#include <sol/sol.hpp>

#include "Events.h"
#include "Types.h"
#include "UserInputManager.h"
#include "mle/common/Consts.h"
#include "mle/common/Utils.h"

namespace mle::window {

/// Configuration for the engine window.
struct Config {
    std::string title;  ///< Window title string.
    vec2i size;         ///< Window size in pixels.
};

/// Creation information passed to the window subsystem.
struct CI {
    Config target_config{.title = DEFAULT_WINDOW_NAME, .size = DEFAULT_WINDOW_SIZE};  ///< Desired window configuration.

    /**
     * @brief Optional Lua configuration table used to override target_config.
     *
     * The table may contain the following fields:
     * - `"title"` (string): Overrides `target_config.title`.
     * - `"size"` (array of 2 integers): Overrides `target_config.size` using [width, height] format.
     * - `"width"` (integer): Overrides `target_config.size.x`.
     * - `"height"` (integer): Overrides `target_config.size.y`.
     */
    sol::object table;
};

/// Initializes the window subsystem with the given configuration.
void init(const CI& ci);

/// Updates window state and polls events. Should be called once per frame.
void update();

/// Shuts down the window and releases resources.
void shutdown();

/// Returns a reference to the window event dispatcher.
ED& getED();

/// Returns a non-owning reference to the internal GLFW window.
GLFWWindowRef getGlfwWindowRef();

/// Returns the current active window configuration.
const Config& getCurrentConfig();

/// Returns the current size of the window in pixels.
vec2i getSize();

/// Returns the current aspect ratio (width / height) of the window.
inline f32 getAspectRatio() {
    return as<f32>(getSize().x) / as<f32>(getSize().y);
}

/// Returns a reference to the user input manager.
UserInputManager& getUIM();
}  // namespace mle::window
