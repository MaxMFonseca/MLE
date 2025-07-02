#include <mle/Entry.inl>

#include "App.h"
#include "scenes/Init.h"

mle::core::CI config() {
    mle::core::CI config;

    config.app_name = "MLECubes";
    // config.init_controller = std::make_unique<init::Controller>();

    // config.registerLuaTypes = registerLuaTypes;
    // config.registerUIElementExtraTypes = registerUIElementExtraTypes();

    config.scene = std::make_unique<mle_cubes::Init>();

    return config;
}
