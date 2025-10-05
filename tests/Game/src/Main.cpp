#include <filesystem>

#include "mle/client/Client.h"
#include "mle/core/Consts.h"
#include "mle/core/Core.h"

int main(int argc, char** argv) {
    std::cout << "/// Hello, Testing 123123123456! ///////////////////////////////////////\n";
    std::cout << "cwd: " << std::filesystem::current_path() << "\n";
    std::cout << "is debug build: " << mle::IS_DEBUG_BUILD << "\n";

    mle::Core::InitInfo core_ii;
    core_ii.argc = argc;
    core_ii.argv = argv;

    mle::Core::i().init(core_ii);

    mle::Client::i().init();
    mle::Client::i().run();

    // mle::shutdown();
}
