#pragma once

#include <filesystem>
#include <iostream>

#include "mle/client/Client.h"
#include "mle/core/Consts.h"
#include "mle/core/Core.h"

int main(int argc, char* argv[]) {
    std::cout << "/// Hello, World! ///////////////////////////////////////\n";
    std::cout << "cwd: " << std::filesystem::current_path() << "\n";
    std::cout << "is debug build: " << mle::IS_DEBUG_BUILD << "\n";
    std::cout << "is client build: " << mle::IS_CLIENT << "\n";

    mle::Core::InitInfo core_ii;
    core_ii.argc = argc;
    core_ii.argv = argv;

    mle::Core::i().init(core_ii);

    mle::Client::i().init();
    mle::Client::i().pushGameLayer(std::make_unique<mle::client::Layer>());
    mle::Client::i().run();

    // mle::shutdown();

    std::cout << "/// Goodbye, World! /// OK! //////////////////////////////\n";

    return 0;
}
