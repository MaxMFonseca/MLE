/**
 * @file RuntimeConfig.h
 * @brief Provides runtime configuration hooks and control flags for the engine.
 *
 * This header exposes a set of runtime configuration utilities used for modifying
 * engine behavior via command-line arguments and runtime switches.
 *
 * TODO: move this to core/
 *
 * @ingroup config
 */

#pragma once

#include <functional>

#include "mle/common/Consts.h"
#include "mle/common/Types.h"

#ifndef MLE_RUNTIME_CONFIG_LOG_LEVEL_FILE_DISABLE
#define MLE_RUNTIME_CONFIG_LOG_LEVEL_FILE
#endif

#ifndef MLE_RUNTIME_CONFIG_LOG_LEVEL_STDOUT_DISABLE
#define MLE_RUNTIME_CONFIG_LOG_LEVEL_STDOUT
#endif

#ifndef MLE_RUNTIME_CONFIG_RENDER_ELEMENT_PADDING_DISABLE
#define MLE_RUNTIME_CONFIG_RENDER_ELEMENT_PADDING
#endif
#undef MLE_RUNTIME_CONFIG_RENDER_ELEMENT_PADDING  // Always disabled for now

#ifndef MLE_RUNTIME_CONFIG_PRESENT_MODE_DISABLE
#define MLE_RUNTIME_CONFIG_PRESENT_MODE
#endif
namespace vk {
enum class PresentModeKHR;
}  // namespace vk

#ifndef MLE_IS_DEBUG_BUILD
#undef MLE_RUNTIME_CONFIG_LOG_LEVEL_STDOUT
#endif

namespace mle::runtime_config {
using ConfigOptionFunction = std::function<void(std::vector<std::string>::iterator, usize)>;
using ConfigOptionEntry = std::pair<std::string, ConfigOptionFunction>;
/**
 * @brief Initializes the runtime configuration with command-line arguments.
 * @param argc Argument count from `main`.
 * @param argv Argument values from `main`.
 * @param user_options A list of user-defined command-line options to parse.
 */
void init(int argc, char** argv, const std::vector<ConfigOptionEntry>& user_options);

#ifdef MLE_RUNTIME_CONFIG_LOG_LEVEL_FILE
/**
 * @brief Gets the current log level for file output.
 */
LogLevel getLogLevelFile();

/**
 * @brief Sets the log level for file output.
 */
void setLogLevelFile(LogLevel level);
#endif

#ifdef MLE_RUNTIME_CONFIG_LOG_LEVEL_STDOUT
/**
 * @brief Gets the current log level for stdout.
 */
LogLevel getLogLevelStdout();

/**
 * @brief Sets the log level for stdout.
 */
void setLogLevelStdout(LogLevel level);

/**
 * @brief Registers a listener to be called when the stdout log level changes.
 */
void signListenerLogLevelStdout(std::function<void(LogLevel)>&& fn);
#endif

#ifdef MLE_RUNTIME_CONFIG_RENDER_ELEMENT_PADDING
/**
 * @brief Checks whether render element padding is enabled.
 */
bool getRenderElementPadding();

/**
 * @brief Enables or disables render element padding.
 */
void setRenderElementPadding(bool render_element_padding);
#endif

#ifdef MLE_RUNTIME_CONFIG_PRESENT_MODE
/**
 * @brief Gets the configured Vulkan present mode.
 */
vk::PresentModeKHR getPresentMode();

/**
 * @brief Sets the Vulkan present mode.
 */
void setPresentMode(vk::PresentModeKHR present_mode);
#endif
}  // namespace mle::runtime_config
