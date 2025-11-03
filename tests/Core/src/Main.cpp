#include <gtest/gtest.h>

#include <filesystem>

#include "mle/client/Client.h"
#include "mle/core/Consts.h"
#include "mle/core/Core.h"
#include "mle/renderer/Renderer.h"

int main(int argc, char** argv) {
    std::cout << "/// Hello, Testing 123123123456! ///////////////////////////////////////\n";
    std::cout << "cwd: " << std::filesystem::current_path() << "\n";
    std::cout << "is debug build: " << mle::IS_DEBUG_BUILD << "\n";
    std::cout << "is client build: " << mle::IS_CLIENT << "\n";

    mle::Core::InitInfo core_ii;
    core_ii.argc = argc;
    core_ii.argv = argv;

    mle::Core::i().init(core_ii);
    mle::Client::i().init();
    mle::Renderer::i().init();

    ::testing::InitGoogleTest(&argc, argv);

    auto test_results = RUN_ALL_TESTS();

    mle::Renderer::i().shutdown();
    mle::Core::i().shutdown();

    std::cout << "/// Bye! ///////////////////////////////////////////////////////////////\n";

    return test_results;
}
