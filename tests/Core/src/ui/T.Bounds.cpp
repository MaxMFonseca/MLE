#include <gtest/gtest.h>

#include "mle/lua/Lua.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Base.h"

TEST(BoundsSchedulingTest, ResizeCallbackLayoutRequestSurvivesCurrentUpdate) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    ui.setRoot(lua.createTable());

    mle::ui::Entt root{ui, ui.getRoot()};
    root.emplace<mle::ui::comp::OnResized>(mle::ui::comp::OnResized{
        .fn = [](const mle::ui::Entt& ew) { ew.requestInternalBoundsUpdate(); },
    });
    root.addFlag<mle::ui::comp::ResizedFlag>();

    ui.boundsSystem().update();

    EXPECT_TRUE(root.has<mle::ui::comp::RequestInternalBoundsUpdateFlag>());
}
