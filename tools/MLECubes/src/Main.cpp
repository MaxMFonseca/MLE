#include <mle/Entry.inl>

mle::core::CI config() {
    mle::core::CI config;

    config.app_name = "MLECubes";
    // config.init_controller = std::make_unique<init::Controller>();

    // config.registerLuaTypes = registerLuaTypes;
    // config.registerUIElementExtraTypes = registerUIElementExtraTypes();

    return config;
}
