#include <gtest/gtest.h>

#include "mle/lua/Lua.h"
#include "mle/ui/Entt.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Relationship.h"
#include "mle/ui/systems/Rendering.h"

namespace {
mle::ui::Entt makeTestEntt(mle::UI& ui) {
    auto entity = ui.getRegistry().create();
    mle::ui::Entt ew{ui, entity};
    ew.emplace<mle::ui::comp::Relationship>();
    return ew;
}

mle::ui::system::Rendering::Packet::Node makeNode(mle::Recti bounds, bool escape_parent_scissor = false) {
    mle::ui::system::Rendering::Packet::Node node{};
    node.bounds.parent_px = bounds;
    node.escape_parent_scissor = escape_parent_scissor;
    return node;
}

mle::ui::system::Rendering::RenderingContext makeContext(mle::Recti parent_scissor, mle::Recti render_target_scissor) {
    mle::ui::system::Rendering::RenderingContext ctx{};
    ctx.parent_scissor = parent_scissor;
    ctx.render_target_scissor = render_target_scissor;
    return ctx;
}
}  // namespace

TEST(RenderingScissorTest, EscapeParentScissorKeyAddsFlag) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    ew.apply("escape_parent_scissor", lua.createObject(true));

    EXPECT_TRUE(ew.has<mle::ui::comp::EscapeParentScissorFlag>());
}

TEST(RenderingScissorTest, EscapeParentScissorFalseRemovesFlag) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    ew.apply("escape_parent_scissor", lua.createObject(true));
    ASSERT_TRUE(ew.has<mle::ui::comp::EscapeParentScissorFlag>());

    ew.apply("escape_parent_scissor", lua.createObject(false));

    EXPECT_FALSE(ew.has<mle::ui::comp::EscapeParentScissorFlag>());
}

TEST(RenderingScissorTest, AliasKeysUseSameFlag) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;

    auto ignore_parent = makeTestEntt(ui);
    ignore_parent.apply("ignore_parent_scissor", lua.createObject(true));
    EXPECT_TRUE(ignore_parent.has<mle::ui::comp::EscapeParentScissorFlag>());

    auto render_outside = makeTestEntt(ui);
    render_outside.apply("render_outside_parent", lua.createObject(true));
    EXPECT_TRUE(render_outside.has<mle::ui::comp::EscapeParentScissorFlag>());
}

TEST(RenderingScissorTest, DefaultNodeClipsToParentScissor) {
    auto node = makeNode(mle::Recti{0, 0, 100, 100});
    auto ctx = makeContext(mle::Recti{25, 25, 50, 50}, mle::Recti{0, 0, 200, 200});

    auto scissor = mle::ui::system::Rendering::computeNodeScissorForTest(node, node.bounds.parent_px, ctx);

    EXPECT_EQ(scissor, (mle::Recti{25, 25, 50, 50}));
}

TEST(RenderingScissorTest, EscapedNodeClipsToRenderTargetInsteadOfParent) {
    auto node = makeNode(mle::Recti{0, 0, 100, 100}, true);
    auto ctx = makeContext(mle::Recti{25, 25, 50, 50}, mle::Recti{0, 0, 200, 200});

    auto scissor = mle::ui::system::Rendering::computeNodeScissorForTest(node, node.bounds.parent_px, ctx);

    EXPECT_EQ(scissor, (mle::Recti{0, 0, 100, 100}));
}

TEST(RenderingScissorTest, EscapedNodeStillClipsToRenderTarget) {
    auto node = makeNode(mle::Recti{-20, -20, 80, 80}, true);
    auto ctx = makeContext(mle::Recti{25, 25, 10, 10}, mle::Recti{0, 0, 50, 50});

    auto scissor = mle::ui::system::Rendering::computeNodeScissorForTest(node, node.bounds.parent_px, ctx);

    EXPECT_EQ(scissor, (mle::Recti{0, 0, 50, 50}));
}

TEST(RenderingScissorTest, EscapedNodeScissorCanBecomeChildParentScope) {
    auto escaped_parent = makeNode(mle::Recti{0, 0, 100, 100}, true);
    auto root_ctx = makeContext(mle::Recti{25, 25, 50, 50}, mle::Recti{0, 0, 200, 200});
    auto escaped_parent_scissor =
        mle::ui::system::Rendering::computeNodeScissorForTest(escaped_parent, escaped_parent.bounds.parent_px, root_ctx);

    auto child = makeNode(mle::Recti{90, 90, 40, 40});
    auto child_ctx = makeContext(escaped_parent_scissor, root_ctx.render_target_scissor);

    auto child_scissor = mle::ui::system::Rendering::computeNodeScissorForTest(child, child.bounds.parent_px, child_ctx);

    EXPECT_EQ(child_scissor, (mle::Recti{90, 90, 10, 10}));
}
