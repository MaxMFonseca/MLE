/**
 * @file Core.h
 * @brief Core lifecycle management and runtime statistics tracking.
 *
 * This file defines the core engine API for controlling the application lifecycle and tracking
 * high-level runtime KPIs.
 *
 * ## How to Use
 *
 * 1. Create a `CreateInfo` struct
 * 2. Call `mle::core::init(info);` to initialize the engine.
 * 3. Call `mle::core::run();` to enter the main loop.
 * 4. Call `mle::core::stop();` to begin the shutdown process.
 *
 * - Use `recordKPIDuration()` to track KPI timings per frame.
 *
 * ## Example
 * @code
 * mle::core::CI info = {
 *     .app_name = "My App",
 *     .registerLuaTypes = [] { registerMyLuaTypes(); },
 *     .init_controller = makeMyController()
 * };
 *
 * mle::core::init(info);
 * mle::core::run();
 * @endcode
 */

#pragma once

#include <chrono>
#include <functional>

#include "mle/common/Result.h"
// #include "mle/ui/Types.h"
#include "mle/common/Utils.h"
#include "mle/window/Events.h"

namespace mle::core {
/// Configuration info for initializing the core engine.
struct CreateInfo {
    std::string app_name;                    ///< Application name
    std::function<void()> registerLuaTypes;  ///< Callback to register Lua bindings before controller init.
    // ui::ControllerHnd init_controller;       ///< Initial UI controller to be activated.
};
using CI = CreateInfo;  ///< Alias for CreateInfo

/// Core state
enum class State : u8 {
    UNINITIALIZED,  ///< Engine has not started initialization.
    INITIALIZING,   ///< Engine is currently initializing.
    INITIALIZED,    ///< Initialization completed, not yet running.
    RUNNING,        ///< Main loop is running.
    STOPPING,       ///< Stop has been requested, shutting down soon.
    STOPPED,        ///< Engine has stopped running, but not shut down.
    SHUTDOWN        ///< Engine has been fully shut down.
};

/// Tracked Key Performance Indicators (KPIs)
enum class SecondKPIType : u8 {
    RENDERING,  ///< Time spent on rendering per second.
    COUNT       ///< Number of KPI types.
};

/// Initializes the engine core using the provided configuration.
void init(CI ci);

/// Runs the main loop until `stop()` is called or an error occurs.
void run();

/// Signals the core to stop after the current frame.
void stop();

/// Enqueue a job for asynchronous execution using a thread pool.
void execAsync(std::function<void(void)>&& func);

/**
 * @brief Unrecoverable error handler. Shuts down the engine immediately.
 *
 * @param msg Error message to log before shutdown.
 */
void unrecoverable(const std::string& msg);

/**
 * @brief Unrecoverable error handler. Shuts down the engine immediately.
 *
 * @tparam Args Variadic template arguments to format the error message.
 * @param args Arguments to format into the error message.
 */
template <typename... Args>
void unrecoverable(fmt::format_string<Args...> fmt_str, Args&&... args) {
    unrecoverable(fmt::format(fmt_str, std::forward<Args>(args)...));
}

/// Returns the total time the engine has been running in milliseconds.
std::chrono::milliseconds getRunningTimeMS();

/// Returns the total time the engine has been running in seconds as f32.
f32 getRunningTimeF32();

/// Adds a duration value to the given KPI for the current second.
void accumulateKPI(SecondKPIType kpi_type, std::chrono::nanoseconds value);
}  // namespace mle::core

namespace fmt {
template <>
struct formatter<mle::core::SecondKPIType> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::core::SecondKPIType val, FormatContext& ctx) const {
        switch (val) {
            case mle::core::SecondKPIType::RENDERING:
                return format_to(ctx.out(), "RENDERING");
            default:
                return format_to(ctx.out(), "UNKNOWN");
        }
    }
};

}  // namespace fmt
