#include "App.h"

#include "mle/lua/Lua.h"
#include "mle/ui/UI.h"

namespace mle_cubes {
void App::init() {
    MLE_C("MLECubes App initializing");

    mle::ui::setNextRoot(mle::lua::require("i/ui/init/layout"));
}

void App::shutdown() {
    MLE_C("MLECubes App shutting down");
}

void App::update() {
    MLE_W("MLECubes App updating");

    static int update_count = 0;
    if (update_count < 35) {
        update_count++;
        if (update_count == 35) {
            mle::ui::setNextRoot(mle::lua::require("i/ui/editor/layout"));
        }
    }
}
}  // namespace mle_cubes
