#include <gtest/gtest.h>

#include <array>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#define private public
#include "mle/window/UserInputManager.h"
#undef private
#pragma clang diagnostic pop

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

TEST(TextTest, BorderThicknessUsesPixelsWithoutAtlasScaling) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto table = lua.createTable();
    table["text"] = "";
    table["border_thickness"] = 1.0F;
    ew.apply("text", table);

    auto& text_renderable = mle::ui::renderable::Text::getFromEntt(ew)->get();
    mle::ui::renderable::TextPacket packet;
    text_renderable.doUpdatePacket(ew, &packet);

    EXPECT_FLOAT_EQ(packet.border_thickness, 1.0F);
}

TEST(TextTest, LuaInputCallbacksReceiveFocusedEntityAndValue) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    int submit_calls = 0;
    int complete_calls = 0;
    std::string submitted_value;
    std::string completed_value;
    entt::entity submitted_entity = entt::null;
    entt::entity completed_entity = entt::null;
    lua.setFunction("text_submit_test", [&](mle::ui::Entt callback_ew, const std::string& value) {
        ++submit_calls;
        submitted_value = value;
        submitted_entity = callback_ew.e();
    });
    lua.setFunction("text_complete_test", [&](mle::ui::Entt callback_ew, const std::string& value) {
        ++complete_calls;
        completed_value = value;
        completed_entity = callback_ew.e();
    });

    auto input = lua.createTable();
    input["on_submit"] = lua.getGlobal<sol::function>("text_submit_test");
    input["on_complete"] = lua.getGlobal<sol::function>("text_complete_test");
    auto table = lua.createTable();
    table["text"] = "Search";
    table["input"] = input;
    ew.apply("text", table);

    auto& text = mle::ui::renderable::Text::getFromEntt(ew)->get();
    text.input_tb->setText(U"i/hero.glb");
    text.input_tb->setFocused(true);

    auto& uim = mle::UserInputManager::i();
    uim.setPressed(mle::Key::ENTER);
    uim.update();
    uim.setReleased(mle::Key::ENTER);
    uim.lateUpdate();
    uim.setPressed(mle::Key::TAB);
    uim.update();
    uim.setReleased(mle::Key::TAB);
    uim.lateUpdate();

    EXPECT_EQ(submit_calls, 1);
    EXPECT_EQ(complete_calls, 1);
    EXPECT_EQ(submitted_value, "i/hero.glb");
    EXPECT_EQ(completed_value, "i/hero.glb");
    EXPECT_EQ(submitted_entity, ew.e());
    EXPECT_EQ(completed_entity, ew.e());
}

TEST(TextTest, InputSetReplacesValueMovesCaretAndPreservesFocus) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto table = lua.createTable();
    table["text"] = "Search";
    table["input"] = true;
    ew.apply("text", table);

    auto& text = mle::ui::renderable::Text::getFromEntt(ew)->get();
    text.input_tb->setText(U"old");
    text.input_tb->setSelection(0, 1);
    text.input_tb->setFocused(true);

    ew.apply("text_input_set", lua.createObject("i/hero.glb#Body"));

    EXPECT_EQ(text.input_tb->getTextUtf8(), "i/hero.glb#Body");
    EXPECT_EQ(text.input_tb->getSelection(), (std::pair<mle::usize, mle::usize>{15, 15}));
    EXPECT_TRUE(text.input_tb->isFocused());
}

TEST(TextTest, ExclusiveInputFocusPreservesTypedValuesAcrossSequentialSelection) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    std::array inputs{
        makeTestEntt(ui),
        makeTestEntt(ui),
        makeTestEntt(ui),
        makeTestEntt(ui),
    };

    auto input = lua.createTable();
    input["text"] = "Resource";
    input["input"] = true;
    for (const auto& ew : inputs) {
        ew.apply("text", input);
    }

    const auto focus = [&](mle::usize selected) {
        for (const auto& ew : inputs) {
            ew.apply("text_input_disable");
        }
        inputs[selected].apply("text_input_enable");
    };
    const auto value = [&](mle::usize index) {
        return mle::ui::renderable::Text::getFromEntt(inputs[index])->get().input_tb->getTextUtf8();
    };
    auto& uim = mle::UserInputManager::i();

    focus(0);
    uim.pushChar(U'M');
    EXPECT_EQ(value(0), "M");
    EXPECT_EQ(value(1), "");
    EXPECT_EQ(value(2), "");
    EXPECT_EQ(value(3), "");

    focus(1);
    uim.pushChar(U'H');
    EXPECT_EQ(value(0), "M");
    EXPECT_EQ(value(1), "H");
    EXPECT_EQ(value(2), "");
    EXPECT_EQ(value(3), "");

    focus(2);
    uim.pushChar(U'A');
    EXPECT_EQ(value(0), "M");
    EXPECT_EQ(value(1), "H");
    EXPECT_EQ(value(2), "A");
    EXPECT_EQ(value(3), "");

    focus(3);
    uim.pushChar(U'N');
    EXPECT_EQ(value(0), "M");
    EXPECT_EQ(value(1), "H");
    EXPECT_EQ(value(2), "A");
    EXPECT_EQ(value(3), "N");

    for (mle::usize index = 0; index < inputs.size(); ++index) {
        const auto& text = mle::ui::renderable::Text::getFromEntt(inputs[index])->get();
        EXPECT_EQ(text.input_tb->isFocused(), index == 3);
    }
}
