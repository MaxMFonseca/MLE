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

TEST(TextTest, DisplayTextUsesPlaceholderOnlyWhileUnfocused) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto table = lua.createTable();
    table["text"] = "Search";
    table["input"] = true;
    ew.apply("text", table);

    auto& text_renderable = mle::ui::renderable::Text::getFromEntt(ew)->get();
    EXPECT_EQ(text_renderable.makeDisplayText(), U"Search");

    text_renderable.input_tb->setFocused(true);
    EXPECT_EQ(text_renderable.makeDisplayText(), U"|");
    EXPECT_EQ(text_renderable.getValue(), "");
}

TEST(TextTest, DisplayTextPlacesCaretAtSelectionEnd) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto table = lua.createTable();
    table["text"] = "Placeholder";
    table["input"] = true;
    ew.apply("text", table);

    auto& text_renderable = mle::ui::renderable::Text::getFromEntt(ew)->get();
    text_renderable.input_tb->setText(U"abcd");
    text_renderable.input_tb->setFocused(true);

    text_renderable.input_tb->setSelection(0, 0);
    EXPECT_EQ(text_renderable.makeDisplayText(), U"|abcd");
    text_renderable.input_tb->setSelection(2, 2);
    EXPECT_EQ(text_renderable.makeDisplayText(), U"ab|cd");
    text_renderable.input_tb->setSelection(4, 4);
    EXPECT_EQ(text_renderable.makeDisplayText(), U"abcd|");
    text_renderable.input_tb->setSelection(1, 3);
    EXPECT_EQ(text_renderable.makeDisplayText(), U"abc|d");
    EXPECT_EQ(text_renderable.getValue(), "abcd");
}

TEST(TextTest, StaticTextDisplayNeverGetsCaret) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    ew.apply("text", lua.createObject("Label"));

    auto& text_renderable = mle::ui::renderable::Text::getFromEntt(ew)->get();
    EXPECT_EQ(text_renderable.makeDisplayText(), U"Label");
}

TEST(TextTest, UpdatePacketUsesInputTextWhenStaticTextIsEmpty) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto table = lua.createTable();
    table["text"] = "";
    table["input"] = true;
    ew.apply("text", table);

    auto& text_renderable = mle::ui::renderable::Text::getFromEntt(ew)->get();
    text_renderable.input_tb->setText(U"typed");
    text_renderable.chars_buffer_needs_update = false;
    text_renderable.setColor(mle::Color::ZERO);

    mle::ui::renderable::TextPacket packet;
    text_renderable.doUpdatePacket(ew, &packet);

    EXPECT_EQ(packet.color, mle::Color::ZERO);
}

TEST(TextTest, UpdatePacketClearsStaleDataWhenInputAndStaticTextAreEmpty) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto table = lua.createTable();
    table["text"] = "";
    table["input"] = true;
    ew.apply("text", table);

    auto& text_renderable = mle::ui::renderable::Text::getFromEntt(ew)->get();
    text_renderable.input_tb->setText(U"");

    text_renderable.input_tb->setFocused(true);
    text_renderable.chars_buffer_needs_update = false;
    text_renderable.setColor(mle::Color::ZERO);

    mle::ui::renderable::TextPacket focused_packet;
    text_renderable.doUpdatePacket(ew, &focused_packet);
    EXPECT_EQ(focused_packet.color, mle::Color::ZERO);

    text_renderable.input_tb->setFocused(false);

    mle::ui::renderable::TextPacket packet;
    packet.per_image_data.resize(1);
    text_renderable.doUpdatePacket(ew, &packet);

    EXPECT_TRUE(packet.per_image_data.empty());
}
