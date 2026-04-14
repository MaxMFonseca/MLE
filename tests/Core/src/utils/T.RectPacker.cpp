#include <gtest/gtest.h>

#include "mle/utils/RectPacker.h"

using namespace mle;  //  NOLINT

TEST(RectPacker, ReturnsEmptyWhenNothingFits) {
    RectPacker rp({0, 0}, {2, 2}, 16);

    EXPECT_EQ(rp.tryPackOne({1, 1}), std::nullopt);

    const std::vector<vec2u> input{{1, 1}, {2, 2}};
    const auto packed = rp.pack(input);
    EXPECT_TRUE(packed.empty());
}

TEST(RectPacker, SingleExactFitConsumesWholeExtent) {
    RectPacker rp({16, 8}, {2, 2}, 4);

    auto pos = rp.tryPackOne({14, 6});
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(*pos, (vec2u{0, 0}));

    EXPECT_EQ(rp.tryPackOne({1, 1}), std::nullopt);
}

TEST(RectPacker, PackStopsAtFirstNonFittingRect) {
    RectPacker rp({32, 16}, {2, 2}, 4);

    const std::vector<vec2u> input{{8, 8}, {40, 1}, {4, 4}};
    const auto packed = rp.pack(input);

    ASSERT_EQ(packed.size(), 1U);
    EXPECT_EQ(packed[0], (vec2u{0, 0}));
}

TEST(RectPacker, PlacesSecondAndThirdRectsToTheRightGivenTypicalSplit) {
    RectPacker rp({32, 16}, {2, 2}, 4);

    const std::vector<vec2u> input{{8, 8}, {8, 8}, {8, 8}};
    const auto packed = rp.pack(input);

    ASSERT_EQ(packed.size(), 3U);
    EXPECT_EQ(packed[0], (vec2u{0, 0}));
    EXPECT_EQ(packed[1], (vec2u{10, 0}));
    EXPECT_EQ(packed[2], (vec2u{20, 0}));
}

TEST(RectPacker, SmallRemaindersAreDiscardedByMinBinSize) {
    RectPacker rp({32, 16}, {2, 2}, 16);

    auto p0 = rp.tryPackOne({14, 14});
    ASSERT_TRUE(p0.has_value());
    EXPECT_EQ(*p0, (vec2u{0, 0}));

    EXPECT_EQ(rp.tryPackOne({8, 4}), std::nullopt);
}

TEST(RectPacker, NothingFitsWhenExtentIsZero) {
    RectPacker rp({0, 0}, {2, 2}, 16);
    EXPECT_EQ(rp.tryPackOne({1, 1}), std::nullopt);
    const std::vector<vec2u> input{{1, 1}, {2, 2}};
    EXPECT_TRUE(rp.pack(input).empty());
}

TEST(RectPacker, ExactFitWithPaddingAtEdges) {
    RectPacker rp({16, 8}, {2, 2}, 4);
    auto pos = rp.tryPackOne({14, 6});
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(*pos, (vec2u{0, 0}));
    EXPECT_EQ(rp.tryPackOne({1, 1}), std::nullopt);
}

TEST(RectPacker, PackStopsAtFirstNonFittingRect2) {
    RectPacker rp({32, 16}, {2, 2}, 4);
    const std::vector<vec2u> input{{8, 8}, {40, 1}, {4, 4}};
    const auto packed = rp.pack(input);
    ASSERT_EQ(packed.size(), 1U);
    EXPECT_EQ(packed[0], (vec2u{0, 0}));
}

TEST(RectPacker, TypicalRightwardPlacementSequenceWithEdgePadding) {
    RectPacker rp({32, 16}, {2, 2}, 4);
    const std::vector<vec2u> input{{8, 8}, {8, 8}, {8, 8}};
    const auto packed = rp.pack(input);
    ASSERT_EQ(packed.size(), 3U);
    EXPECT_EQ(packed[0], (vec2u{0, 0}));
    EXPECT_EQ(packed[1], (vec2u{10, 0}));
    EXPECT_EQ(packed[2], (vec2u{20, 0}));
}

TEST(RectPacker, SmallRemaindersPrunedByMinBinSize) {
    RectPacker rp({32, 16}, {2, 2}, 16);
    auto p0 = rp.tryPackOne({14, 14});
    ASSERT_TRUE(p0.has_value());
    EXPECT_EQ(*p0, (vec2u{0, 0}));
    EXPECT_EQ(rp.tryPackOne({8, 4}), std::nullopt);
}

TEST(RectPacker, EqualRemaindersTieLeadsToRightPlacementOnSecondInsertNoPaddingCase) {
    RectPacker rp({20, 20}, {0, 0}, 1);

    auto p0 = rp.tryPackOne({10, 10});
    ASSERT_TRUE(p0.has_value());
    EXPECT_EQ(*p0, (vec2u{0, 0}));

    auto p1 = rp.tryPackOne({10, 10});
    ASSERT_TRUE(p1.has_value());
    EXPECT_EQ(*p1, (vec2u{10, 0}));
}

TEST(RectPacker, EdgePaddingReducesCountComparedToNoPadding) {
    RectPacker rp_pad({32, 32}, {2, 2}, 1);
    RectPacker rp_nopad({32, 32}, {0, 0}, 1);

    std::vector<vec2u> req(16, vec2u{8, 8});

    const auto with_pad = rp_pad.pack(req);
    const auto no_pad = rp_nopad.pack(req);

    EXPECT_LT(with_pad.size(), no_pad.size());
    EXPECT_EQ(no_pad.size(), 16U);
}

TEST(RectPacker, ManySmallRectsFillFirstRowThenNextWithPadding) {
    RectPacker rp({32, 16}, {2, 2}, 1);

    const std::vector<vec2u> input{{8, 8}, {8, 8}, {8, 8}, {8, 8}};
    const auto packed = rp.pack(input);
    ASSERT_EQ(packed.size(), 3U);
    EXPECT_EQ(packed[0], (vec2u{0, 0}));
    EXPECT_EQ(packed[1], (vec2u{10, 0}));
    EXPECT_EQ(packed[2], (vec2u{20, 0}));
}

TEST(RectPacker, OrderMattersAndIsObservable) {
    RectPacker rp({30, 14}, {2, 2}, 1);

    const std::vector<vec2u> bad_order{{8, 8}, {50, 1}, {8, 8}};
    const auto packed_bad = rp.pack(bad_order);
    ASSERT_EQ(packed_bad.size(), 1U);

    RectPacker rp2({30, 14}, {2, 2}, 1);
    const std::vector<vec2u> good_order{{8, 8}, {8, 8}, {50, 1}};
    const auto packed_good = rp2.pack(good_order);
    ASSERT_EQ(packed_good.size(), 2U);
    EXPECT_EQ(packed_good[0], (vec2u{0, 0}));
    EXPECT_NE(packed_good[1], (vec2u{0, 0}));
}
