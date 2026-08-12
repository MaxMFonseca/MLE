#include <gtest/gtest.h>

#include "mle/lua/Lua.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"
#include "mle/ui/components/Base.h"

TEST(TargetBoundTest, ExplicitZeroPixelsReplacePreviousValue) {
    mle::ui::TargetBound bound;
    bound.set("350px");
    bound.set("0px");
    EXPECT_EQ(bound.type, mle::ui::TargetBound::Type::PX);
    EXPECT_FLOAT_EQ(bound.val, 0.0F);
}

TEST(TargetBoundTest, ExplicitZeroPreservesRequestedSuffixType) {
    mle::ui::TargetBound percent;
    percent.set("0%");
    mle::ui::TargetBound flex;
    flex.set("0f");
    EXPECT_EQ(percent.type, mle::ui::TargetBound::Type::RELATIVE);
    EXPECT_FLOAT_EQ(percent.val, 0.0F);
    EXPECT_EQ(flex.type, mle::ui::TargetBound::Type::FLEX_SHARE);
    EXPECT_FLOAT_EQ(flex.val, 0.0F);
}

TEST(TargetBoundTest, SuffixOnlyFitStillParsesAsFit) {
    mle::ui::TargetBound fit;
    fit.set("fit");
    EXPECT_EQ(fit.type, mle::ui::TargetBound::Type::FIT);
}

TEST(TargetBoundTest, SuffixOnlyShorthandsUseTheirRequestedTypes) {
    mle::ui::TargetBound percent;
    percent.set("%");
    mle::ui::TargetBound flex;
    flex.set("f");
    mle::ui::TargetBound pixels;
    pixels.set("px");

    EXPECT_EQ(percent.type, mle::ui::TargetBound::Type::RELATIVE);
    EXPECT_FLOAT_EQ(percent.val, 1.0F);
    EXPECT_EQ(flex.type, mle::ui::TargetBound::Type::FLEX_SHARE);
    EXPECT_FLOAT_EQ(flex.val, 1.0F);
    EXPECT_EQ(pixels.type, mle::ui::TargetBound::Type::PX);
    EXPECT_FLOAT_EQ(pixels.val, 1.0F);
}

TEST(TargetBoundTest, ExplicitZeroTableFormsPreserveRequestedTypes) {
    mle::Lua lua;
    lua.init();

    auto pixels_table = lua.createTable();
    pixels_table[1] = 0.0F;
    pixels_table[2] = "px";
    mle::ui::TargetBound pixels;
    pixels.set(pixels_table);

    auto percent_table = lua.createTable();
    percent_table[1] = 0.0F;
    percent_table[2] = "%";
    mle::ui::TargetBound percent;
    percent.set(percent_table);

    auto flex_table = lua.createTable();
    flex_table[1] = 0.0F;
    flex_table[2] = "f";
    mle::ui::TargetBound flex;
    flex.set(flex_table);

    EXPECT_EQ(pixels.type, mle::ui::TargetBound::Type::PX);
    EXPECT_FLOAT_EQ(pixels.val, 0.0F);
    EXPECT_EQ(percent.type, mle::ui::TargetBound::Type::RELATIVE);
    EXPECT_FLOAT_EQ(percent.val, 0.0F);
    EXPECT_EQ(flex.type, mle::ui::TargetBound::Type::FLEX_SHARE);
    EXPECT_FLOAT_EQ(flex.val, 0.0F);
}

TEST(BoundsSchedulingTest, NonRootResizeCallbackFiresFromLayoutAndSchedulesNextUpdate) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;

    auto child = lua.createTable();
    child["name"] = "responsive_layout";
    child["size"] = 1.0F;
    child["list"] = lua.createTable();
    auto children = lua.createTable();
    children[1] = child;
    auto root_table = lua.createTable();
    root_table["size_x"] = "800px";
    root_table["size_y"] = "600px";
    root_table["list"] = lua.createTable();
    root_table["c"] = children;
    ui.setRoot(root_table);

    std::array tree{std::string_view{"responsive_layout"}};
    auto responsive = ui.getE(tree).value();
    int resize_calls = 0;
    responsive.emplace<mle::ui::comp::OnResized>(mle::ui::comp::OnResized{
        .fn = [&](const mle::ui::Entt& ew) {
            ++resize_calls;
            ew.requestInternalBoundsUpdate();
        },
    });

    ui.boundsSystem().update();

    EXPECT_EQ(resize_calls, 1);
    EXPECT_TRUE(responsive.has<mle::ui::comp::RequestInternalBoundsUpdateFlag>());
}
