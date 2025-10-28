/**
 * @file Consts.h
 * @brief Compile-time constants and build configuration flags for the engine.
 *
 * This header is meant to fetch all static configuration from the build system,
 * but for now it simply exposes a few constants to the engine without most toggles.
 */

#pragma once

#include "Types.h"

#ifndef NDEBUG
#define MLE_IS_DEBUG_BUILD
#endif

#ifndef MLE_MAX_LOG_LEVEL
#ifdef MLE_IS_DEBUG_BUILD
#define MLE_MAX_LOG_LEVEL TRACE
#else
#define MLE_MAX_LOG_LEVEL INFO
#endif
#endif

#ifndef MLE_DEFAULT_LOG_LEVEL_STDOUT
#define MLE_DEFAULT_LOG_LEVEL_STDOUT INFO
#endif

#ifndef MLE_LOG_FILE_NAME
#define MLE_LOG_FILE_NAME false
#endif

namespace mle {
/// Indicates whether the current build is a debug build.
#ifdef MLE_IS_DEBUG_BUILD
constexpr bool IS_DEBUG_BUILD = true;
#else
constexpr bool IS_DEBUG_BUILD = false;
#endif

///< Indicates if this is a client build.
#if !defined(MLE_NO_CLIENT) || defined(MLE_HAS_CLIENT_BUILD)
constexpr bool IS_CLIENT = true;
#define MLE_IS_CLIENT
#else
constexpr bool IS_CLIENT = false;
#endif

/// Maximum log level compiled into the binary.
constexpr LogLevel MAX_LOG_LEVEL = LogLevel::MLE_MAX_LOG_LEVEL;

/// Default log level for standard output.
constexpr LogLevel DEFAULT_LOG_LEVEL_STDOUT = LogLevel::MLE_DEFAULT_LOG_LEVEL_STDOUT;

/// Whether to include file names in log output.
static constexpr bool LOG_FILE_NAME = MLE_LOG_FILE_NAME;

/// The name of the engine.
constexpr auto ENGINE_NAME = "MLE";

/// Engine version major number.
constexpr auto ENGINE_VERSION_MAJOR = 0;
/// Engine version minor number.
constexpr auto ENGINE_VERSION_MINOR = 0;
/// Engine version patch number.
constexpr auto ENGINE_VERSION_PATCH = 1;

/// Engine tick rate.
constexpr auto TICK_RATE = 1'000'000'000ns / 60;

/// Default font height.
constexpr int DEFAULT_FONT_HEIGHT = 64;

/// Default window size.
constexpr vec2i DEFAULT_WINDOW_SIZE{1280, 720};

/// Default window name.
constexpr auto DEFAULT_WINDOW_NAME = "MLE";

/// Resource paths used by the engine.
struct ResPath {
    static constexpr auto RES = "res";            ///< Base resource path.
    static constexpr auto MLE_SUBDIR = "mle";     ///< Subdirectory for engine resources.
    static constexpr auto USER_SUBDIR = "i";      ///< Subdirectory for user resources.
    static constexpr auto LUA = "lua";            ///< Path to Lua scripts.
    static constexpr auto TEXTURES = "textures";  ///< Path to texture assets.
    static constexpr auto SHADERS = "shaders";    ///< Path to shader files.
    static constexpr auto FONTS = "fonts";        ///< Path to font files.
    static constexpr auto MODELS = "models";      ///< Path to 3D model files.
    static constexpr auto SOUNDS = "sounds";      ///< Path to sound files.
};
}  // namespace mle
