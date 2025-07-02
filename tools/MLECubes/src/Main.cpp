#include <mle/Entry.inl>

#include "App.h"

mle::core::CI config() {
    mle::core::CI config;

    config.app_name = "MLECubes";
    // config.init_controller = std::make_unique<init::Controller>();

    // config.registerLuaTypes = registerLuaTypes;
    // config.registerUIElementExtraTypes = registerUIElementExtraTypes();

    config.app.init = mle_cubes::app::init;
    config.app.shutdown = mle_cubes::app::shutdown;
    config.app.update = mle_cubes::app::update;
    config.app.render = mle_cubes::app::render;

    return config;
}
