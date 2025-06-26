#include "App.h"

#include "mle/lua/Lua.h"
#include "mle/ui/UI.h"

namespace mle_cubes {
void App::init() {
    MLE_C("MLECubes App initializing");

    mle::ui::setNextRoot(mle::lua::require("ui/init/layout"));
}

void App::shutdown() {
    MLE_C("MLECubes App shutting down");
}

void App::update() {
    MLE_W("MLECubes App updating");
}
}  // namespace mle_cubes
