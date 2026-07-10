#include <gtest/gtest.h>

#include "mle/lua/Lua.h"
#include "mle/ui/Entt.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/FreeContainer.h"
#include "mle/ui/components/Relationship.h"

namespace {
mle::ui::Entt makeTestEntt(mle::UI& ui) {
    auto entity = ui.getRegistry().create();
    mle::ui::Entt ew{ui, entity};
    ew.emplace<mle::ui::comp::Relationship>();
    return ew;
}

sol::table getScrollState(const mle::ui::Entt& ew) {
    auto state = ew.getKey("scroll");
    EXPECT_TRUE(state.is<sol::table>());
    return state.as<sol::table>();
}
}  // namespace

TEST(ScrollStateTest, GetterReturnsCurrentAndMaximum) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto config = lua.createTable();
    config["max_scroll_y"] = 120;
    config["current_scroll_y"] = 45;
    ew.apply("free", config);

    auto state = getScrollState(ew);
    EXPECT_EQ(state.get<int>("current_scroll_y"), 45);
    EXPECT_EQ(state.get<int>("max_scroll_y"), 120);
}

TEST(ScrollStateTest, CurrentOffsetClampsToRange) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto config = lua.createTable();
    config["max_scroll_y"] = 100;
    config["current_scroll_y"] = 150;
    ew.apply("free", config);
    EXPECT_EQ(getScrollState(ew).get<int>("current_scroll_y"), 100);

    config = lua.createTable();
    config["current_scroll_y"] = -10;
    ew.apply("free", config);
    EXPECT_EQ(getScrollState(ew).get<int>("current_scroll_y"), 0);
}

TEST(ScrollStateTest, LowerMaximumReclampsCurrentOffset) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto config = lua.createTable();
    config["max_scroll_y"] = 100;
    config["current_scroll_y"] = 80;
    ew.apply("free", config);

    config = lua.createTable();
    config["max_scroll_y"] = 30;
    ew.apply("free", config);

    auto state = getScrollState(ew);
    EXPECT_EQ(state.get<int>("current_scroll_y"), 30);
    EXPECT_EQ(state.get<int>("max_scroll_y"), 30);
}

TEST(ScrollStateTest, NegativeMaximumBecomesZero) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto config = lua.createTable();
    config["max_scroll_y"] = -1;
    ew.apply("free", config);

    auto state = getScrollState(ew);
    EXPECT_EQ(state.get<int>("current_scroll_y"), 0);
    EXPECT_EQ(state.get<int>("max_scroll_y"), 0);
}

TEST(ScrollStateTest, ContentOverflowResetsAfterContentShrinks) {
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    mle::ui::comp::ContentOverflow::setFromSizes(ew, {140, 250}, {100, 100});
    ASSERT_TRUE(ew.has<mle::ui::comp::ContentOverflow>());
    EXPECT_EQ(ew.get<mle::ui::comp::ContentOverflow>().overflow_x, 40);
    EXPECT_EQ(ew.get<mle::ui::comp::ContentOverflow>().overflow_y, 150);

    mle::ui::comp::ContentOverflow::setFromSizes(ew, {80, 90}, {100, 100});
    EXPECT_EQ(ew.get<mle::ui::comp::ContentOverflow>().overflow_x, 0);
    EXPECT_EQ(ew.get<mle::ui::comp::ContentOverflow>().overflow_y, 0);
}
