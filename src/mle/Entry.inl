/**
 * @file Entry.inl
 * @brief Entry point template for MLE-based applications.
 *
 * This file defines the main entry point for a project using the MLE engine.
 * To use it, simply include this file in your main compilation unit:
 *
 * @code{.cpp}
 * #include "Entry.inl"
 * @endcode
 *
 * You must define a function with the following signature in your code:
 *
 * @code{.cpp}
 * mle::core::CI config();
 * @endcode
 *
 * This function returns an instance of `mle::core::CI` (Core Init), which provides
 * the engine with startup parameters such as window title, resolution, and feature toggles.
 *
 * ---
 *
 * ### Responsibilities:
 * - Prints diagnostic information (current working directory, build type).
 * - Initializes runtime configuration from command-line arguments.
 * - Initializes the engine core using user-supplied configuration.
 * - Starts the main application loop.
 * - Cleans up on exit.
 *
 * ---
 *
 * ### Example
 * @code{.cpp}
 * // In some user .cpp file:
 * #include "mle/core/CI.h"
 *
 * mle::core::CI config() {
 *     mle::core::CI ci;
 *     ci.name = "My Game";
 *     ci.width = 1280;
 *     ci.height = 720;
 *     return ci;
 * }
 *
 * // Include this after defining config()
 * #include "Entry.inl"
 * @endcode
 *
 * ---
 *
 * @warning Include this file only once in your project. It defines `main()`.
 * Multiple inclusion across translation units will result in linker errors.
 */

#pragma once

#include <iostream>

#include "mle/common/RuntimeConfig.h"
#include "mle/core/Core.h"
// #include "mle/ui/Controller.h"

mle::core::CI config();  // NOLINT to be defined in the user code

int main(int argc, char* argv[]) {
    std::cout << "/// Hello, World! ///////////////////////////////////////\n";
    std::cout << "cwd: " << std::filesystem::current_path() << "\n";
    std::cout << "is debug build: " << mle::IS_DEBUG_BUILD << "\n";

    mle::runtime_config::init(argc, argv, {});  // TODO: Allow user to use this too

    mle::core::init(config());
    mle::core::run();

    std::cout << "/// Goodbye, World! /// OK! //////////////////////////////\n";

    return 0;
}
