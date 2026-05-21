#include <gtest/gtest.h>

#include "mle/lua/Lua.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/Entt.h"
#include "mle/ui/components/Relationship.h"
#include "mle/ui/renderable/Sprite.h"

namespace {
mle::ui::Entt makeTestEntt(mle::UI& ui) {
    auto entity = ui.getRegistry().create();
    mle::ui::Entt ew{ui, entity};
    ew.emplace<mle::ui::comp::Relationship>();
    return ew;
}
}  // namespace

TEST(SpriteTest, LuaTableParsesNormalizedUvRegion) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto table = lua.createTable();
    table["texture"] = "missing-atlas.png";
    table["uv"] = lua.createTable();
    table["uv"][1] = 0.25F;
    table["uv"][2] = 0.5F;
    table["uv_size"] = lua.createTable();
    table["uv_size"][1] = 0.125F;
    table["uv_size"][2] = 0.25F;

    mle::ui::renderable::Sprite sprite;
    sprite.set(ew, table);

    EXPECT_EQ(sprite.uv, mle::vec2f(0.25F, 0.5F));
    EXPECT_EQ(sprite.uv_size, mle::vec2f(0.125F, 0.25F));
}

TEST(SpriteTest, LuaTableSupportsPositionalTextureWithUvRegion) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto table = lua.createTable();
    table[1] = "missing-atlas.png";
    table["uv"] = lua.createTable();
    table["uv"][1] = 0.1F;
    table["uv"][2] = 0.2F;
    table["uv_size"] = lua.createTable();
    table["uv_size"][1] = 0.3F;
    table["uv_size"][2] = 0.4F;

    mle::ui::renderable::Sprite sprite;
    sprite.set(ew, table);

    EXPECT_EQ(sprite.uv, mle::vec2f(0.1F, 0.2F));
    EXPECT_EQ(sprite.uv_size, mle::vec2f(0.3F, 0.4F));
}

TEST(SpriteTest, LuaTableParsesPixelUvRegionFromImageExtent) {
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

    mle::ui::renderable::Sprite sprite;
    sprite.set(ew, table);

    EXPECT_EQ(sprite.uv, mle::vec2f(2.0F / mle::as<mle::f32>(extent.x), 4.0F / mle::as<mle::f32>(extent.y)));
    EXPECT_EQ(sprite.uv_size, mle::vec2f(8.0F / mle::as<mle::f32>(extent.x), 16.0F / mle::as<mle::f32>(extent.y)));
}

TEST(SpriteTest, PixelUvRegionOverridesNormalizedUvRegion) {
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

    mle::ui::renderable::Sprite sprite;
    sprite.set(ew, table);

    EXPECT_EQ(sprite.uv, mle::vec2f(2.0F / mle::as<mle::f32>(extent.x), 4.0F / mle::as<mle::f32>(extent.y)));
    EXPECT_EQ(sprite.uv_size, mle::vec2f(8.0F / mle::as<mle::f32>(extent.x), 16.0F / mle::as<mle::f32>(extent.y)));
}

TEST(SpriteTest, PacketUpdateCopiesUvRegion) {
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    mle::ui::renderable::Sprite sprite;
    sprite.uv = mle::vec2f(0.25F, 0.5F);
    sprite.uv_size = mle::vec2f(0.125F, 0.25F);

    mle::ui::renderable::SpritePacket packet;
    sprite.doUpdatePacket(ew, &packet);

    EXPECT_EQ(packet.uv, mle::vec2f(0.25F, 0.5F));
    EXPECT_EQ(packet.uv_size, mle::vec2f(0.125F, 0.25F));
}

TEST(SpriteTest, UvSizeAffectsCalculatedAspectWhenNotFit) {
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    mle::ui::renderable::Sprite sprite;
    sprite.source = mle::ui::renderable::SpriteSource::IMAGE;
    sprite.image = mle::Renderer::i().textureCache().getDefaultTexture();
    sprite.uv_size = mle::vec2f(0.25F, 0.5F);

    mle::vec2u bounds = sprite.calculateBounds(ew, mle::vec2u{100, 100});

    EXPECT_EQ(bounds, mle::vec2u(50, 100));
}
