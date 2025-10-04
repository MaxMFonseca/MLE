#pragma once

#include <filesystem>
#include <iostream>

#include "mle/core/Consts.h"
#include "mle/core/Core.h"

int main(int argc, char* argv[]) {
    std::cout << "/// Hello, World! ///////////////////////////////////////\n";
    std::cout << "cwd: " << std::filesystem::current_path() << "\n";
    std::cout << "is debug build: " << mle::IS_DEBUG_BUILD << "\n";

    mle::CoreInitInfo core_ii;
    core_ii.argc = argc;
    core_ii.argv = argv;

    mle::init(core_ii);
    // mle::shutdown();

    std::cout << "/// Goodbye, World! /// OK! //////////////////////////////\n";

    return 0;
}
