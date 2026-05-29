#include <gtest/gtest.h>

#include "mle/client/Client.h"
#include "mle/ui/Entt.h"
#include "mle/ui/components/Relationship.h"

namespace {
mle::ui::Entt makeTestEntt(mle::UI& ui) {
    auto entity = ui.getRegistry().create();
    mle::ui::Entt ew{ui, entity};
    ew.emplace<mle::ui::comp::Relationship>();
    return ew;
}
}  // namespace

TEST(TagsTest, ApplyAndHasTag) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    // Test single tag apply
    ew.apply("tags", lua.createObject("tag1"));
    EXPECT_TRUE(ew.hasTag("tag1"));
    EXPECT_FALSE(ew.hasTag("tag2"));

    // Test multiple tags apply
    sol::table table = lua.createTable();
    table[1] = "tag1";
    table[2] = "tag2";
    ew.apply("tags", table);
    EXPECT_TRUE(ew.hasTag("tag1"));
    EXPECT_TRUE(ew.hasTag("tag2"));
    EXPECT_FALSE(ew.hasTag("tag3"));

    // Test getKey (tags)
    auto tags_obj = ew.getKey("tags");
    ASSERT_TRUE(tags_obj.is<sol::table>());
    sol::table tags_table = tags_obj.as<sol::table>();
    EXPECT_EQ(tags_table.size(), 2);
    EXPECT_EQ(tags_table.get<std::string>(1), "tag1");
    EXPECT_EQ(tags_table.get<std::string>(2), "tag2");
}

TEST(TagsTest, GetChildrenWithTagRecursive) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    
    auto root = makeTestEntt(ui);
    auto child1 = makeTestEntt(ui);
    auto child2 = makeTestEntt(ui);
    auto grandchild = makeTestEntt(ui);

    root.getRelationship().addChild(root, child1.e());
    root.getRelationship().addChild(root, child2.e());
    child1.getRelationship().addChild(child1, grandchild.e());

    child1.apply("tags", lua.createObject("enemy"));
    grandchild.apply("tags", lua.createObject("enemy"));
    child2.apply("tags", lua.createObject("friendly"));

    auto enemies = root.getChildrenWithTagRecursive("enemy");
    EXPECT_EQ(enemies.size(), 2);

    auto friendlies = root.getChildrenWithTagRecursive("friendly");
    EXPECT_EQ(friendlies.size(), 1);
    EXPECT_EQ(friendlies[0].e(), child2.e());

    auto ghosts = root.getChildrenWithTagRecursive("ghost");
    EXPECT_EQ(ghosts.size(), 0);
}
