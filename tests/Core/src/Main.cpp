#include <gtest/gtest.h>

#include <filesystem>

#include "mle/core/Consts.h"
#include "mle/core/Core.h"

int main(int argc, char** argv) {
    std::cout << "/// Hello, Testing 123123123456! ///////////////////////////////////////\n";
    std::cout << "cwd: " << std::filesystem::current_path() << "\n";
    std::cout << "is debug build: " << mle::IS_DEBUG_BUILD << "\n";

    mle::core::InitInfo core_ii;
    core_ii.argc = argc;
    core_ii.argv = argv;

    mle::core::init(core_ii);

    ::testing::InitGoogleTest(&argc, argv);

    auto test_results = RUN_ALL_TESTS();
    // mle::shutdown();
    return test_results;
}
