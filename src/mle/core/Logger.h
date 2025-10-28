/**
 * @file Logger.h
 * @brief Logging subsystem built on spdlog
 *
 * Example usage:
 * @code
 * MLE_I("Game started: version {}", version);
 * MLE_VD(score); // Logs "score: <value>" with DEBUG level
 * MLE_I(MLE_3V(player, score, level)); // Logs "player: <value>, score: <value>, level: <value>" with INFO level
 * @endcode
 */
#pragma once

#define SPDLOG_ACTIVE_LEVEL MLE_MAX_LOG_LEVEL
#include <spdlog/common.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "Consts.h"
#include "RuntimeConfig.h"

// NOLINTBEGIN

/**
 * @defgroup LogMacros Logging Macros
 * @brief Macros for logging at various severity levels.
 * @{
 *
 * Example usage:
 * @code
 * MLE_C("Hello world!"); // Hello world! with CRITICAL level
 * MLE_I("Game started: version {}", version); // Game started: version <value> with INFO level
 * MLE_W(MLE_3V(v1, v2, v3)); // logs "v1: <value>, v2: <value>, v3: <value>" with WARN level
 * MLE_VT(v1); // logs "v1: <value>" with TRACE level
 * MLE_LOG_THIS; // logs the address of the current object with DEBUG level
 * @endcode
 */
#define MLE_LOG(s, ...) SPDLOG_##s(__VA_ARGS__)        ///< Base logging macro that dispatches to spdlog with a severity level.
#define MLE_LOG_T(...) MLE_LOG(TRACE, __VA_ARGS__)     ///< Logs a TRACE message.
#define MLE_T(...) MLE_LOG_T(__VA_ARGS__)              ///< Logs a TRACE message.
#define MLE_LOG_D(...) MLE_LOG(DEBUG, __VA_ARGS__)     ///< Logs a DEBUG message.
#define MLE_D(...) MLE_LOG_D(__VA_ARGS__)              ///< Logs a DEBUG message.
#define MLE_LOG_I(...) MLE_LOG(INFO, __VA_ARGS__)      ///< Logs an INFO message.
#define MLE_I(...) MLE_LOG_I(__VA_ARGS__)              ///< Logs an INFO message.
#define MLE_LOG_W(...) MLE_LOG(WARN, __VA_ARGS__)      ///< Logs a WARN message.
#define MLE_W(...) MLE_LOG_W(__VA_ARGS__)              ///< Logs a WARN message.
#define MLE_LOG_E(...) MLE_LOG(ERROR, __VA_ARGS__)     ///< Logs an ERROR message.
#define MLE_E(...) MLE_LOG_E(__VA_ARGS__)              ///< Logs an ERROR message.
#define MLE_LOG_C(...) MLE_LOG(CRITICAL, __VA_ARGS__)  ///< Logs a CRITICAL message.
#define MLE_C(...) MLE_LOG_C(__VA_ARGS__)              ///< Logs a CRITICAL message.

/// Expands to a format string and arguments for one variable: "name: value"
#define MLE_1V(e) "{}: {}", #e, (e)
/// Expands to a format string and arguments for two variables: "name: value, name: value"
#define MLE_2V(e1, e2) "{}: {}, {}: {}", #e1, (e1), #e2, (e2)
/// Expands to a format string and arguments for three variables
#define MLE_3V(e1, e2, e3) "{}: {}, {}: {}, {}: {}", #e1, (e1), #e2, (e2), #e3, (e3)
/// Expands to a format string and arguments for four variables
#define MLE_4V(e1, e2, e3, e4) "{}: {}, {}: {}, {}: {}, {}: {}", #e1, (e1), #e2, (e2), #e3, (e3), #e4, (e4)
/// Expands to a format string and arguments for five variables
#define MLE_5V(e1, e2, e3, e4, e5) "{}: {}, {}: {}, {}: {}, {}: {}, {}: {}", #e1, (e1), #e2, (e2), #e3, (e3), #e4, (e4), #e5, (e5)
/// Expands to a format string and arguments for six variables
#define MLE_6V(e1, e2, e3, e4, e5, e6) "{}: {}, {}: {}, {}: {}, {}: {}, {}: {}, {}: {}", #e1, (e1), #e2, (e2), #e3, (e3), #e4, (e4), #e5, (e5), #e6, (e6)

#define MLE_LOG_VALUE_T(e) MLE_T("{}: {}", #e, (e))  ///< Logs a variable and its value at TRACE level.
#define MLE_VT(e) MLE_LOG_VALUE_T(e)                 ///< Logs a variable and its value at TRACE level.
#define MLE_LOG_VALUE_D(e) MLE_D("{}: {}", #e, (e))  ///< Logs a variable and its value at DEBUG level.
#define MLE_VD(e) MLE_LOG_VALUE_D(e)                 ///< Logs a variable and its value at DEBUG level.
#define MLE_LOG_VALUE_I(e) MLE_I("{}: {}", #e, (e))  ///< Logs a variable and its value at INFO level.
#define MLE_VI(e) MLE_LOG_VALUE_I(e)                 ///< Logs a variable and its value at INFO level.
#define MLE_LOG_VALUE_W(e) MLE_W("{}: {}", #e, (e))  ///< Logs a variable and its value at WARN level.
#define MLE_VW(e) MLE_LOG_VALUE_W(e)                 ///< Logs a variable and its value at WARN level.
#define MLE_LOG_VALUE_E(e) MLE_E("{}: {}", #e, (e))  ///< Logs a variable and its value at ERROR level.
#define MLE_VE(e) MLE_LOG_VALUE_E(e)                 ///< Logs a variable and its value at ERROR level.
#define MLE_LOG_VALUE_C(e) MLE_C("{}: {}", #e, (e))  ///< Logs a variable and its value at CRITICAL level.
#define MLE_VC(e) MLE_LOG_VALUE_C(e)                 ///< Logs a variable and its value at CRITICAL level.

#define MLE_LOG_THIS_T MLE_VT((void*)this)  ///< Logs the current `this` pointer at TRACE level.
#define MLE_LOG_THIS_I MLE_VI((void*)this)  ///< Logs the current `this` pointer at INFO level.
#define MLE_LOG_THIS_D MLE_VD((void*)this)  ///< Logs the current `this` pointer at DEBUG level.
#define MLE_LOG_THIS_W MLE_VW((void*)this)  ///< Logs the current `this` pointer at WARN level.
#define MLE_LOG_THIS_E MLE_VE((void*)this)  ///< Logs the current `this` pointer at ERROR level.
#define MLE_LOG_THIS_C MLE_VC((void*)this)  ///< Logs the current `this` pointer at CRITICAL level.
#define MLE_LOG_THIS MLE_LOG_THIS_D         ///< Logs the current `this` pointer at DEBUG level (default).
/// @}

// NOLINTEND

namespace mle {
class Logger {
    MLE_SINGLETON(Logger);

  public:
    void init();
    void shutdown();

    std::shared_ptr<spdlog::sinks::sink> getFileSink() { return file_sink_; }
    std::shared_ptr<spdlog::sinks::sink> getStdOutSink() { return stdout_sink_; }

  private:
    std::shared_ptr<spdlog::sinks::sink> file_sink_;
    std::shared_ptr<spdlog::sinks::sink> stdout_sink_;
    mle::RuntimeConfigListener file_level_listener_;
    mle::RuntimeConfigListener stdout_level_listener_;
};
}  // namespace mle
