#include <gtest/gtest.h>

#include "mle/lua/Lua.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/Entt.h"
#include "mle/ui/components/Animation.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Renderable.h"
#include "mle/ui/components/Bounds.h"
#include "mle/ui/components/Relationship.h"
#include "mle/ui/renderable/Sprite.h"
#include "mle/ui/renderable/Text.h"

namespace {
mle::ui::Entt makeTestEntt(mle::UI& ui) {
    auto entity = ui.getRegistry().create();
    mle::ui::Entt ew{ui, entity};
    ew.emplace<mle::ui::comp::Relationship>();
    return ew;
}
}  // namespace

TEST(AnimationTest, ParsesTweenTrackFromLua) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto track = lua.createTable();
    track["target"] = "render_scale";
    track["from"] = 1.0F;
    track["to"] = 2.0F;
    track["duration"] = 0.5F;
    track["ease"] = "out_quad";
    track["loop"] = true;
    track["yoyo"] = true;

    auto tracks = lua.createTable();
    tracks[1] = track;
    auto anim_table = lua.createTable();
    anim_table["tracks"] = tracks;

    mle::ui::comp::Animation::apply(ew, anim_table);

    const auto& animation = ew.get<mle::ui::comp::Animation>();
    ASSERT_EQ(animation.tracks.size(), 1);
    EXPECT_EQ(animation.tracks[0].target, mle::ui::comp::AnimationTarget::RENDER_SCALE);
    EXPECT_FLOAT_EQ(animation.tracks[0].from.number, 1.0F);
    EXPECT_FLOAT_EQ(animation.tracks[0].to.number, 2.0F);
    EXPECT_FLOAT_EQ(animation.tracks[0].duration, 0.5F);
    EXPECT_EQ(animation.tracks[0].ease, mle::ui::comp::AnimationEase::OUT_QUAD);
    EXPECT_TRUE(animation.tracks[0].loop);
    EXPECT_TRUE(animation.tracks[0].yoyo);
}

TEST(AnimationTest, EasingReturnsExpectedValues) {
    using enum mle::ui::comp::AnimationEase;

    EXPECT_FLOAT_EQ(mle::ui::comp::Animation::ease(LINEAR, 0.5F), 0.5F);
    EXPECT_FLOAT_EQ(mle::ui::comp::Animation::ease(IN_QUAD, 0.5F), 0.25F);
    EXPECT_FLOAT_EQ(mle::ui::comp::Animation::ease(OUT_QUAD, 0.5F), 0.75F);
    EXPECT_FLOAT_EQ(mle::ui::comp::Animation::ease(IN_OUT_QUAD, 0.25F), 0.125F);
    EXPECT_FLOAT_EQ(mle::ui::comp::Animation::ease(IN_OUT_QUAD, 0.75F), 0.875F);
}

TEST(AnimationTest, TickingRenderScaleTweenAppliesInterpolatedValue) {
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    mle::ui::comp::Animation animation;
    animation.tracks.push_back({
        .target = mle::ui::comp::AnimationTarget::RENDER_SCALE,
        .from = mle::ui::comp::AnimationValue{.number = 1.0F},
        .to = mle::ui::comp::AnimationValue{.number = 2.0F},
        .duration = 1.0F,
    });

    animation.tick(ew, 0.5F);

    ASSERT_TRUE(ew.has<mle::ui::comp::RenderScale>());
    EXPECT_FLOAT_EQ(ew.get<mle::ui::comp::RenderScale>().scale, 1.5F);
}

TEST(AnimationTest, TickingSizeTweenAppliesInterpolatedValue) {
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    mle::ui::TargetBound from_tb; from_tb.set(100.0F);
    mle::ui::TargetBound to_tb; to_tb.set(200.0F);

    mle::ui::comp::Animation animation;
    animation.tracks.push_back({
        .target = mle::ui::comp::AnimationTarget::SIZE_X,
        .from = mle::ui::comp::AnimationValue{.kind = mle::ui::comp::AnimationValueKind::TARGET_BOUND, .target_bound = from_tb},
        .to = mle::ui::comp::AnimationValue{.kind = mle::ui::comp::AnimationValueKind::TARGET_BOUND, .target_bound = to_tb},
        .duration = 1.0F,
    });

    animation.tick(ew, 0.5F);

    ASSERT_TRUE(ew.has<mle::ui::comp::TargetSize>());
    EXPECT_FLOAT_EQ(ew.get<mle::ui::comp::TargetSize>().x.val, 150.0F);
}

TEST(AnimationTest, LoopingYoyoTweenReversesOnAlternateCycle) {
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    mle::ui::comp::Animation animation;
    animation.tracks.push_back({
        .target = mle::ui::comp::AnimationTarget::RENDER_SCALE,
        .from = mle::ui::comp::AnimationValue{.number = 1.0F},
        .to = mle::ui::comp::AnimationValue{.number = 2.0F},
        .duration = 1.0F,
        .loop = true,
        .yoyo = true,
    });

    animation.tick(ew, 1.25F);

    ASSERT_TRUE(ew.has<mle::ui::comp::RenderScale>());
    EXPECT_FLOAT_EQ(ew.get<mle::ui::comp::RenderScale>().scale, 1.75F);
}

TEST(AnimationTest, ParsesTypewriterAnimation) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto typewriter = lua.createTable();
    typewriter["cps"] = 24.0F;
    typewriter["start_delay"] = 0.2F;
    auto anim_table = lua.createTable();
    anim_table["typewriter"] = typewriter;

    mle::ui::comp::Animation::apply(ew, anim_table);

    const auto& animation = ew.get<mle::ui::comp::Animation>();
    ASSERT_TRUE(animation.typewriter.has_value());
    EXPECT_FLOAT_EQ(animation.typewriter->cps, 24.0F);
    EXPECT_FLOAT_EQ(animation.typewriter->start_delay, 0.2F);
}

TEST(AnimationTest, ParsesSpriteFrameSizeFromLuaTable) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto sprite = lua.createTable();
    sprite["frame_size"] = lua.createTable();
    sprite["frame_size"][1] = 32;
    sprite["frame_size"][2] = 32;
    sprite["frames"] = 4;
    sprite["fps"] = 6.0F;

    auto anim_table = lua.createTable();
    anim_table["sprite"] = sprite;

    mle::ui::comp::Animation::apply(ew, anim_table);

    const auto& animation = ew.get<mle::ui::comp::Animation>();
    ASSERT_TRUE(animation.sprite.has_value());
    EXPECT_EQ(animation.sprite->frame_size, mle::vec2u(32, 32));
    EXPECT_EQ(animation.sprite->frames, 4);
    EXPECT_FLOAT_EQ(animation.sprite->fps, 6.0F);
}

TEST(AnimationTest, SpriteAnimationAdvancesUvRegion) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto sprite_impl = std::make_unique<mle::ui::renderable::Sprite>();
    sprite_impl->setImage(ew, mle::Renderer::i().textureCache().getDefaultTexture());
    ew.emplace<mle::ui::comp::Renderable>(std::move(sprite_impl));

    mle::ui::comp::Animation animation;
    animation.sprite = mle::ui::comp::SpriteAnimation{
        .frame_size = {5, 5},
        .frames = 4,
        .fps = 2.0F,
        .loop = true,
    };

    animation.tick(ew, 0.5F);

    auto& renderable = ew.get<mle::ui::comp::Renderable>();
    auto& sprite = *mle::as<mle::ui::renderable::Sprite*>(renderable.impl.get());
    EXPECT_EQ(sprite.uv, mle::vec2f(0.5F, 0.0F));
    EXPECT_EQ(sprite.uv_size, mle::vec2f(0.5F, 0.5F));
}

TEST(AnimationTest, TypewriterUpdatesVisibleCharsWithoutChangingText) {
    mle::Lua lua;
    lua.init();
    mle::UI ui;
    auto ew = makeTestEntt(ui);

    auto text_table = lua.createTable();
    text_table["text"] = "Hello";
    text_table["visible_chars"] = 0;
    mle::ui::renderable::Text::apply(ew, text_table);

    mle::ui::comp::Animation animation;
    animation.typewriter = mle::ui::comp::TypewriterAnimation{.cps = 4.0F};

    animation.tick(ew, 0.5F);

    auto text = mle::ui::renderable::Text::getFromEntt(ew);
    ASSERT_TRUE(text.has_value());
    EXPECT_EQ(text->get().visible_chars, 2);
    EXPECT_EQ(text->get().getValue(), "Hello");
}
