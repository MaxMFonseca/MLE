#include "gtest/gtest.h"
#include "mle/utils/Justify.h"

using namespace mle;  // NOLINT

namespace {
template <typename T>
void expectVecEq(const std::vector<T>& got, const std::vector<T>& exp) {
    ASSERT_EQ(got.size(), exp.size());
    for (size_t i = 0; i < got.size(); ++i) {
        EXPECT_EQ(got[i], exp[i]);
    }
}

template <typename T>
void expectVecNear(const std::vector<T>& got, const std::vector<T>& exp, T eps) {
    ASSERT_EQ(got.size(), exp.size());
    for (size_t i = 0; i < got.size(); ++i) {
        EXPECT_NEAR(got[i], exp[i], eps);
    }
}
}  // namespace

TEST(Justify, NoWrapBasic) {
    std::vector<f32> sizes{10.F, 20.F, 30.F};
    auto res = Justify<f32>::noWrap(sizes, 5.F);
    expectVecNear(res.first, {0.F, 15.F, 40.F}, 1e-5F);
}

TEST(Justify, WrapOneLineAllFitStart) {
    std::vector<f32> sizes{10.F, 10.F, 10.F};
    auto line = Justify<f32>::justifyUntilOverflow(sizes, 5.F, Justify<f32>::LineMode::START, Justify<f32>::LineMode::START, 100.F);
    expectVecNear(line, {0.F, 15.F, 30.F}, 1e-5F);
}

TEST(Justify, WrapOneLineAllFitLastLineCenter) {
    std::vector<f32> sizes{10.F, 10.F, 10.F};
    auto line = Justify<f32>::justifyUntilOverflow(sizes, 5.F, Justify<f32>::LineMode::START, Justify<f32>::LineMode::CENTER, 100.F);
    expectVecNear(line, {30.F, 45.F, 60.F}, 1e-5F);
}

TEST(Justify, WrapOneLineFirstTooLarge) {
    std::vector<f32> sizes{120.F, 10.F, 10.F};
    auto line = Justify<f32>::justifyUntilOverflow(sizes, 5.F, Justify<f32>::LineMode::START, Justify<f32>::LineMode::CENTER, 100.F);
    expectVecNear(line, {0.F}, 1e-5F);
}

TEST(Justify, WrapOneLineMinGapStopsBeforeOverflow) {
    std::vector<f32> sizes{40.F, 40.F, 40.F};
    auto line = Justify<f32>::justifyUntilOverflow(sizes, 10.F, Justify<f32>::LineMode::START, Justify<f32>::LineMode::START, 100.F);
    expectVecNear(line, {0.F, 50.F}, 1e-5F);
}

TEST(Justify, WrapLinesMultiLine) {
    std::vector<f32> sizes{30.F, 30.F, 30.F};
    auto lines = Justify<f32>::wrap(sizes, 10.F, Justify<f32>::LineMode::START, Justify<f32>::LineMode::CENTER, 80.F);
    ASSERT_EQ(lines.size(), 2U);
    expectVecNear(lines[0], {0.F, 40.F}, 1e-5F);
    expectVecNear(lines[1], {25.F}, 1e-5F);
}

TEST(Justify, NoWrapEmpty) {
    std::vector<f32> sizes{};
    auto res = Justify<f32>::noWrap(sizes, 5.F);
    EXPECT_TRUE(res.first.empty());
}

TEST(Justify, JustifySpaceBetweenAllFit) {
    std::vector<f32> sizes{10.F, 10.F, 10.F};
    auto line = Justify<f32>::justifyUntilOverflow(sizes, 5.F, Justify<f32>::LineMode::SPACE_BETWEEN, Justify<f32>::LineMode::SPACE_BETWEEN, 100.F);
    expectVecNear(line, {0.F, 45.F, 90.F}, 1e-5F);
}

TEST(Justify, JustifySpaceAroundSingleItemCenters) {
    std::vector<f32> sizes{20.F};
    auto line = Justify<f32>::justifyUntilOverflow(sizes, 5.F, Justify<f32>::LineMode::SPACE_AROUND, Justify<f32>::LineMode::SPACE_AROUND, 100.F);
    expectVecNear(line, {40.F}, 1e-5F);
}

TEST(Justify, JustifySpaceEvenlySingleItemCenters) {
    std::vector<f32> sizes{20.F};
    auto line = Justify<f32>::justifyUntilOverflow(sizes, 5.F, Justify<f32>::LineMode::SPACE_EVENLY, Justify<f32>::LineMode::SPACE_EVENLY, 100.F);
    expectVecNear(line, {40.F}, 1e-5F);
}

TEST(Justify, JustifySpaceEvenlyThreeItems) {
    std::vector<f32> sizes{10.F, 10.F, 10.F};
    auto line = Justify<f32>::justifyUntilOverflow(sizes, 5.F, Justify<f32>::LineMode::SPACE_EVENLY, Justify<f32>::LineMode::SPACE_EVENLY, 70.F);
    expectVecNear(line, {10.F, 30.F, 50.F}, 1e-5F);
}

TEST(Justify, JustifyEndExactSlack) {
    std::vector<f32> sizes{10.F, 10.F, 10.F};
    auto line = Justify<f32>::justifyUntilOverflow(sizes, 5.F, Justify<f32>::LineMode::END, Justify<f32>::LineMode::END, 45.F);
    expectVecNear(line, {5.F, 20.F, 35.F}, 1e-5F);
}

TEST(Justify, JustifyCenterExactSlack) {
    std::vector<f32> sizes{10.F, 10.F, 10.F};
    auto line = Justify<f32>::justifyUntilOverflow(sizes, 5.F, Justify<f32>::LineMode::CENTER, Justify<f32>::LineMode::CENTER, 45.F);
    expectVecNear(line, {2.5F, 17.5F, 32.5F}, 1e-5F);
}

TEST(Justify, WrapOneLineExactFitStart) {
    std::vector<f32> sizes{30.F, 40.F};
    auto line = Justify<f32>::justifyUntilOverflow(sizes, 10.F, Justify<f32>::LineMode::START, Justify<f32>::LineMode::START, 80.F);
    expectVecNear(line, {0.F, 40.F}, 1e-5F);
}

TEST(Justify, WrapLinesLastLineCentered) {
    std::vector<f32> sizes{25.F, 25.F, 25.F};
    auto lines = Justify<f32>::wrap(sizes, 10.F, Justify<f32>::LineMode::START, Justify<f32>::LineMode::CENTER, 70.F);
    ASSERT_EQ(lines.size(), 2U);
    expectVecNear(lines[0], {0.F, 35.F}, 1e-5F);
    expectVecNear(lines[1], {22.5F}, 1e-5F);
}

TEST(Justify, WrapOneLineStopsWhenMinGapWouldOverflow) {
    std::vector<f32> sizes{40.F, 40.F, 40.F};
    auto line = Justify<f32>::justifyUntilOverflow(sizes, 20.F, Justify<f32>::LineMode::START, Justify<f32>::LineMode::START, 100.F);
    expectVecNear(line, {0.F, 60.F}, 1e-5F);
}

TEST(Justify, WrapOneLineTooSmallForSecondItem) {
    std::vector<f32> sizes{30.F, 30.F};
    auto line = Justify<f32>::justifyUntilOverflow(sizes, 20.F, Justify<f32>::LineMode::SPACE_EVENLY, Justify<f32>::LineMode::SPACE_EVENLY, 70.F);
    EXPECT_EQ(line.size(), 1U);
    expectVecNear(line, {20.F}, 1e-5F);
}

TEST(Justify, SpaceBetweenSingleItemStaysAtStart) {
    std::vector<f32> sizes{20.F};
    auto line = Justify<f32>::justifyUntilOverflow(sizes, 5.F, Justify<f32>::LineMode::SPACE_BETWEEN, Justify<f32>::LineMode::SPACE_BETWEEN, 100.F);
    expectVecNear(line, {0.F}, 1e-5F);
}

TEST(Justify, IntegerTypeCenterUsesIntegerDivision) {
    std::vector<int> sizes{10, 10, 10};
    auto line = Justify<int>::justifyUntilOverflow(sizes, 5, Justify<int>::LineMode::CENTER, Justify<int>::LineMode::CENTER, 46);
    expectVecEq(line, std::vector<int>({3, 18, 33}));
}

TEST(Justify, NoWrapBasicInt) {
    std::vector<int> sizes{10, 20, 30};
    auto res = Justify<int>::noWrap(sizes, 5);
    expectVecEq(res.first, std::vector<int>({0, 15, 40}));
}

TEST(Justify, WrapOneLineAllFitStartInt) {
    std::vector<int> sizes{10, 10, 10};
    auto line = Justify<int>::justifyUntilOverflow(sizes, 5, Justify<int>::LineMode::START, Justify<int>::LineMode::START, 100);
    expectVecEq(line, std::vector<int>({0, 15, 30}));
}

TEST(Justify, WrapOneLineAllFitLastLineCenterInt) {
    std::vector<int> sizes{10, 10, 10};
    auto line = Justify<int>::justifyUntilOverflow(sizes, 5, Justify<int>::LineMode::START, Justify<int>::LineMode::CENTER, 100);
    expectVecEq(line, std::vector<int>({30, 45, 60}));
}

TEST(Justify, WrapOneLineFirstTooLargeInt) {
    std::vector<int> sizes{120, 10, 10};
    auto line = Justify<int>::justifyUntilOverflow(sizes, 5, Justify<int>::LineMode::START, Justify<int>::LineMode::CENTER, 100);
    expectVecEq(line, std::vector<int>({0}));
}

TEST(Justify, WrapOneLineMinGapStopsBeforeOverflowInt) {
    std::vector<int> sizes{40, 40, 40};
    auto line = Justify<int>::justifyUntilOverflow(sizes, 10, Justify<int>::LineMode::START, Justify<int>::LineMode::START, 100);
    expectVecEq(line, std::vector<int>({0, 50}));
}

TEST(Justify, WrapLinesMultiLineInt) {
    std::vector<int> sizes{30, 30, 30};
    auto lines = Justify<int>::wrap(sizes, 10, Justify<int>::LineMode::START, Justify<int>::LineMode::CENTER, 80);
    ASSERT_EQ(lines.size(), 2U);
    expectVecEq(lines[0], std::vector<int>({0, 40}));
    expectVecEq(lines[1], std::vector<int>({25}));
}

TEST(Justify, ExactFitAllThree) {
    std::vector<int> sizes{30, 40, 10};
    auto line = Justify<int>::justifyUntilOverflow(sizes, 10, Justify<int>::LineMode::START, Justify<int>::LineMode::START, 100);
    expectVecEq(line, std::vector<int>({0, 40, 90}));
}

TEST(Justify, TightLineOnlyFirstFitsNoFollowingGap) {
    std::vector<int> sizes{10, 10};
    auto line = Justify<int>::justifyUntilOverflow(sizes, 7, Justify<int>::LineMode::START, Justify<int>::LineMode::START, 16);
    expectVecEq(line, std::vector<int>({0}));
}

TEST(Justify, LastLineUsesModeLastLineCenter) {
    std::vector<int> sizes{30, 30, 30};
    auto lines = Justify<int>::wrap(sizes, 10, Justify<int>::LineMode::START, Justify<int>::LineMode::CENTER, 80);
    ASSERT_EQ(lines.size(), 2U);
    expectVecEq(lines[0], std::vector<int>({0, 40}));
    expectVecEq(lines[1], std::vector<int>({25}));
}

TEST(Justify, SpaceBetween3x10on100) {
    std::vector<int> sizes{10, 10, 10};
    auto line = Justify<int>::justifyUntilOverflow(sizes, 5, Justify<int>::LineMode::SPACE_BETWEEN, Justify<int>::LineMode::SPACE_BETWEEN, 100);
    expectVecEq(line, std::vector<int>({0, 45, 90}));
}

TEST(Justify, SpaceAround3x10on100) {
    std::vector<int> sizes{10, 10, 10};
    auto line = Justify<int>::justifyUntilOverflow(sizes, 5, Justify<int>::LineMode::SPACE_AROUND, Justify<int>::LineMode::SPACE_AROUND, 100);
    expectVecEq(line, std::vector<int>({11, 44, 77}));
}

TEST(Justify, SpaceEvenly3x10on100) {
    std::vector<int> sizes{10, 10, 10};
    auto line = Justify<int>::justifyUntilOverflow(sizes, 5, Justify<int>::LineMode::SPACE_EVENLY, Justify<int>::LineMode::SPACE_EVENLY, 100);
    expectVecEq(line, std::vector<int>({17, 44, 71}));
}

TEST(Justify, WrapLinesZeroGapInt) {
    std::vector<int> sizes{5, 5, 5, 5, 5};
    auto lines = Justify<int>::wrap(sizes, 0, Justify<int>::LineMode::START, Justify<int>::LineMode::CENTER, 12);
    ASSERT_EQ(lines.size(), 3U);
    expectVecEq(lines[0], std::vector<int>({0, 5}));
    expectVecEq(lines[1], std::vector<int>({0, 5}));
    expectVecEq(lines[2], std::vector<int>({3}));
}

TEST(Justify, WrapLinesZeroGapInt2) {
    std::vector<int> sizes{5, 5, 5, 5, 5, 5};
    auto lines = Justify<int>::wrap(sizes, 0, Justify<int>::LineMode::START, Justify<int>::LineMode::CENTER, 12);
    ASSERT_EQ(lines.size(), 3U);
    expectVecEq(lines[0], std::vector<int>({0, 5}));
    expectVecEq(lines[1], std::vector<int>({0, 5}));
    expectVecEq(lines[2], std::vector<int>({1, 6}));
}

TEST(Justify, NoWrapSingleInt) {
    std::vector<int> sizes{17};
    auto res = Justify<int>::noWrap(sizes, 9);
    expectVecEq(res.first, std::vector<int>({0}));
}

TEST(Justify, WrapOneLineExactFitBoundaryInt) {
    std::vector<int> sizes{25, 10, 15};
    auto line = Justify<int>::justifyUntilOverflow(sizes, 5, Justify<int>::LineMode::START, Justify<int>::LineMode::START, 60);
    expectVecEq(line, std::vector<int>({0, 30, 45}));
}

TEST(Justify, WrapOneLineStopsBeforeGapInt) {
    std::vector<int> sizes{10, 10};
    auto line = Justify<int>::justifyUntilOverflow(sizes, 7, Justify<int>::LineMode::START, Justify<int>::LineMode::START, 16);
    expectVecEq(line, std::vector<int>({0}));
}
