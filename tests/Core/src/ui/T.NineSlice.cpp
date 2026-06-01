#include <gtest/gtest.h>

#include "mle/lua/Lua.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/Entt.h"
#include "mle/ui/components/Relationship.h"
#include "mle/ui/renderable/NineSlice.h"

namespace {
mle::ui::Entt makeTestEntt(mle::UI& ui) {
    auto entity = ui.getRegistry().create();
    mle::ui::Entt ew{ui, entity};
    ew.emplace<mle::ui::comp::Relationship>();
    return ew;
}

void expectSliceEq(const mle::ui::PaddingPx& actual, const mle::ui::PaddingPx& expected) {
    EXPECT_EQ(actual.t, expected.t);
    EXPECT_EQ(actual.b, expected.b);
    EXPECT_EQ(actual.l, expected.l);
    EXPECT_EQ(actual.r, expected.r);
}
}  // namespace

TEST(NineSliceTest, LuaTableParsesUniformPixelSlice) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto table = lua.createTable();
    table["texture"] = "missing-panel.png";
    table["slice"] = "8px";

    mle::ui::renderable::NineSlice nine_slice;
    nine_slice.set(ew, table);

    expectSliceEq(nine_slice.slice_px, {8, 8, 8, 8});
}

TEST(NineSliceTest, LuaTableParsesPerSidePixelSlice) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto slice = lua.createTable();
    slice["t"] = 3;
    slice["b"] = 5;
    slice["l"] = 7;
    slice["r"] = 11;

    auto table = lua.createTable();
    table["texture"] = "missing-panel.png";
    table["slice"] = slice;

    mle::ui::renderable::NineSlice nine_slice;
    nine_slice.set(ew, table);

    expectSliceEq(nine_slice.slice_px, {3, 5, 7, 11});
}

TEST(NineSliceTest, LuaTableParsesPixelUvRegionFromImageExtent) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);
    auto* image = mle::Renderer::i().textureCache().getDefaultTexture();
    const auto extent = image->getExtent();

    auto table = lua.createTable();
    table["image"] = image;
    table["uv_px"] = lua.createTable();
    table["uv_px"][1] = 2.0F;
    table["uv_px"][2] = 4.0F;
    table["uv_size_px"] = lua.createTable();
    table["uv_size_px"][1] = 8.0F;
    table["uv_size_px"][2] = 16.0F;

    mle::ui::renderable::NineSlice nine_slice;
    nine_slice.set(ew, table);

    EXPECT_EQ(nine_slice.uv, mle::vec2f(2.0F / mle::as<mle::f32>(extent.x), 4.0F / mle::as<mle::f32>(extent.y)));
    EXPECT_EQ(nine_slice.uv_size, mle::vec2f(8.0F / mle::as<mle::f32>(extent.x), 16.0F / mle::as<mle::f32>(extent.y)));
}

TEST(NineSliceTest, PixelUvRegionOverridesNormalizedUvRegion) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);
    auto* image = mle::Renderer::i().textureCache().getDefaultTexture();
    const auto extent = image->getExtent();

    auto table = lua.createTable();
    table["image"] = image;
    table["uv"] = lua.createTable();
    table["uv"][1] = 0.1F;
    table["uv"][2] = 0.2F;
    table["uv_size"] = lua.createTable();
    table["uv_size"][1] = 0.3F;
    table["uv_size"][2] = 0.4F;
    table["uv_px"] = lua.createTable();
    table["uv_px"][1] = 2.0F;
    table["uv_px"][2] = 4.0F;
    table["uv_size_px"] = lua.createTable();
    table["uv_size_px"][1] = 8.0F;
    table["uv_size_px"][2] = 16.0F;

    mle::ui::renderable::NineSlice nine_slice;
    nine_slice.set(ew, table);

    EXPECT_EQ(nine_slice.uv, mle::vec2f(2.0F / mle::as<mle::f32>(extent.x), 4.0F / mle::as<mle::f32>(extent.y)));
    EXPECT_EQ(nine_slice.uv_size, mle::vec2f(8.0F / mle::as<mle::f32>(extent.x), 16.0F / mle::as<mle::f32>(extent.y)));
}

TEST(NineSliceTest, PacketUpdateCopiesSliceState) {
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    mle::ui::renderable::NineSlice nine_slice;
    nine_slice.uv = mle::vec2f(0.25F, 0.5F);
    nine_slice.uv_size = mle::vec2f(0.125F, 0.25F);
    nine_slice.slice_px = {3, 5, 7, 11};

    mle::ui::renderable::NineSlicePacket packet;
    nine_slice.doUpdatePacket(ew, &packet);

    EXPECT_EQ(packet.uv, mle::vec2f(0.25F, 0.5F));
    EXPECT_EQ(packet.uv_size, mle::vec2f(0.125F, 0.25F));
    expectSliceEq(packet.slice_px, {3, 5, 7, 11});
}

TEST(NineSliceTest, FitReturnsMaxSize) {
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    mle::ui::renderable::NineSlice nine_slice;
    nine_slice.fit = true;

    EXPECT_EQ(nine_slice.calculateBounds(ew, mle::vec2u(120, 80)), mle::vec2u(120, 80));
}

TEST(NineSliceTest, UvSizeAffectsCalculatedAspectWhenNotFit) {
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    mle::ui::renderable::NineSlice nine_slice;
    nine_slice.source = mle::ui::renderable::NineSliceSource::IMAGE;
    nine_slice.image = mle::Renderer::i().textureCache().getDefaultTexture();
    nine_slice.uv_size = mle::vec2f(0.25F, 0.5F);

    mle::vec2u bounds = nine_slice.calculateBounds(ew, mle::vec2u{100, 100});

    EXPECT_EQ(bounds, mle::vec2u(50, 100));
}
