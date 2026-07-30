#include <gtest/gtest.h>

#include "mle/audio/SourcePlaybackState.h"

namespace mle::audio {
TEST(SourcePlaybackState, NonSpatialIsCenteredAndListenerRelative) {
    const PlayParams params{
        .position = {3.0F, -2.0F, 9.0F},
        .velocity = {-4.0F, 5.0F, 6.0F},
        .spatial = false,
    };

    const auto state = sourcePlaybackState(params);

    EXPECT_TRUE(state.relative);
    EXPECT_EQ(state.position, vec3f{});
    EXPECT_EQ(state.velocity, vec3f{});
}

TEST(SourcePlaybackState, SpatialUsesRequestedWorldState) {
    const PlayParams params{
        .position = {3.0F, -2.0F, 9.0F},
        .velocity = {-4.0F, 5.0F, 6.0F},
        .spatial = true,
    };

    const auto state = sourcePlaybackState(params);

    EXPECT_FALSE(state.relative);
    EXPECT_EQ(state.position, params.position);
    EXPECT_EQ(state.velocity, params.velocity);
}

TEST(SourcePlaybackState, SpatialThenNonSpatialResetsEveryField) {
    auto state = sourcePlaybackState(PlayParams{
        .position = {1.0F, 2.0F, 3.0F},
        .velocity = {4.0F, 5.0F, 6.0F},
        .spatial = true,
    });
    state = sourcePlaybackState(PlayParams{
        .position = {7.0F, 8.0F, 9.0F},
        .velocity = {-1.0F, -2.0F, -3.0F},
        .spatial = false,
    });

    EXPECT_TRUE(state.relative);
    EXPECT_EQ(state.position, vec3f{});
    EXPECT_EQ(state.velocity, vec3f{});
}

TEST(SourcePlaybackState, NonSpatialThenSpatialRestoresEveryField) {
    auto state = sourcePlaybackState(PlayParams{.spatial = false});
    const PlayParams spatial{
        .position = {7.0F, 8.0F, 9.0F},
        .velocity = {-1.0F, -2.0F, -3.0F},
        .spatial = true,
    };
    state = sourcePlaybackState(spatial);

    EXPECT_FALSE(state.relative);
    EXPECT_EQ(state.position, spatial.position);
    EXPECT_EQ(state.velocity, spatial.velocity);
}
}  // namespace mle::audio
