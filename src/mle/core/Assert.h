/**
 * @file Assert.h
 * @brief Assertion and unreachable code handling macros.
 *
 * This header defines macros for logging and asserting unreachable or invalid code paths,
 * customized for debug and release builds. In debug mode, assertions trigger failures with
 * informative logs. In release mode, unreachable code paths use `std::unreachable()`.
 */

#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <version>

#include "Logger.h"

/**
 * @defgroup AssertMacros Assertion and Unreachable Macros
 * @brief Macros for asserting conditions and marking unreachable code paths.
 *
 * These macros help enforce invariants in both debug and release builds, with appropriate
 * logging and fail-fast mechanisms.
 * @{
 */

/**
 * @def MLE_UNREACHABLE_LOG(...)
 * @brief Logs a message indicating unreachable code has been executed.
 *
 * In debug builds, this macro logs the provided message and triggers an assertion failure.
 * In release builds, it invokes std::unreachable(), indicating that the code path should be unreachable.
 * SO BE CAREFUL! Use this only for UNREACHABLE code.
 *
 * @param ... The message to log.
 */
#ifndef NDEBUG
#define MLE_UNREACHABLE_LOG(...)     \
    {                                \
        MLE_C(__VA_ARGS__);          \
        assert(!"Unreachable code"); \
    }
#else
#define MLE_UNREACHABLE_LOG(...) std::unreachable();
#endif
// TODO: Maybe Core::i().unrecoverable(__VA_ARGS__); This is worse. But better for trace in user builds...

/**
 * @def MLE_UNREACHABLE_TODO
 * @brief Marks a code section as a TODO and indicates it is currently unreachable.
 */
#define MLE_TODO MLE_UNREACHABLE_LOG("TODO")

/**
 * @def MLE_UNREACHABLE
 * @brief Indicates that a code path is unreachable.
 * BE CAREFUL! Use this only for UNREACHABLE code.
 */
#define MLE_UNREACHABLE MLE_UNREACHABLE_LOG("Unreachable code reached.")

/**
 * @def MLE_ASSERT_LOG(cond, ...)
 * @brief Asserts a condition and logs a message if the assertion fails.
 *
 * @param cond The condition to assert.
 * @param ... The message to log if the assertion fails.
 */
#define MLE_ASSERT_LOG(cond, ...)         \
    if (!(cond)) {                        \
        MLE_UNREACHABLE_LOG(__VA_ARGS__); \
    }

/**
 * @def MLE_ASSERT(cond)
 * @brief Asserts a condition and logs a default message if the assertion fails.
 *
 * @param cond The condition to assert.
 */
#define MLE_ASSERT(cond) MLE_ASSERT_LOG(cond, "Assertion failed: {}", #cond)

#define MLE_ASSERT_RESULT(result)                                               \
    {                                                                           \
        auto aux_result = result;                                               \
        MLE_ASSERT_LOG(!isError(aux_result), "Result is error: {}", aux_result) \
    }

/// @}
