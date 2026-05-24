#include <gtest/gtest.h>

#include "mle/lua/Lua.h"
#include "mle/ui/Entt.h"
#include "mle/ui/components/Relationship.h"
#include "mle/ui/renderable/Text.h"

namespace {
mle::ui::Entt makeTestEntt(mle::UI& ui) {
    auto entity = ui.getRegistry().create();
    mle::ui::Entt ew{ui, entity};
    ew.emplace<mle::ui::comp::Relationship>();
    return ew;
}
}  // namespace

TEST(TextTest, LuaGetterReturnsStaticText) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    ew.apply("text", lua.createObject("Pilot"));

    auto text = ew.getKey("text");

    ASSERT_TRUE(text.is<std::string>());
    EXPECT_EQ(text.as<std::string>(), "Pilot");
}

TEST(TextTest, LuaGetterPrefersInputTextWhenPresent) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto table = lua.createTable();
    table["text"] = "Name";
    table["input"] = true;
    ew.apply("text", table);

    auto& text_renderable = mle::ui::renderable::Text::getFromEntt(ew)->get();
    text_renderable.input_tb->setText(U"Max");

    auto text = ew.getKey("text");

    ASSERT_TRUE(text.is<std::string>());
    EXPECT_EQ(text.as<std::string>(), "Max");
}
