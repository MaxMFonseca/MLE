#include <filesystem>
#include <iostream>

#include "layers/Init.h"
#include "layers/ModelTest.h"
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

    mle::Client::i().pushGameLayer(std::make_unique<mle::user::InitLayer>());
    mle::Client::i().run();

    mle::Renderer::i().shutdown();
    mle::Core::i().shutdown();

    // TODO: MAYBE: mle::Core::i().shutdown(); // leaking intentionally for now

    std::cout << "/// Bye! ///////////////////////////////////////////////////////////////\n";
}
