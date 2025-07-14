#include "Init.h"

#include "Editor.h"
#include "mle/core/Core.h"
#include "mle/lua/Lua.h"
#include "mle/ui/UI.h"

namespace mle_cubes {
void Init::init() {
    MLE_I("Init scene initializing");

    mle::ui::setNextRoot("i/ui/init/layout");
}

void Init::update() {  // NOLINT
    static int update_count = 0;
    if (update_count < 35) {
        update_count++;
        if (update_count == 35) {
            core::setNextScene(std::make_unique<Editor>());
        }
    }
}
}  // namespace mle_cubes
