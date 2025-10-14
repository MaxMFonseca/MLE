// tests/RectTests.cpp
#include <gtest/gtest.h>

#include <algorithm>

#include "mle/core/Logger.h"
#include "mle/math/Types.h"    // as<T>, u32, i32, f32
#include "mle/math/Types2D.h"  // Rect, Rectf, Recti, vec2f, vec2i
#include "mle/utils/Utils.h"

using namespace mle;  // NOLINT

namespace {
void expectVec2Near(vec2f a, vec2f b, f32 eps = 1e-6F) {
    EXPECT_NEAR(a.x, b.x, eps);
    EXPECT_NEAR(a.y, b.y, eps);
}
void expectRectNear(const Rectf& r, vec2f pos, vec2f size, f32 eps = 1e-6F) {
    expectVec2Near(r.pos(), pos, eps);
    expectVec2Near(r.size(), size, eps);
}
void expectRectEq(const Recti& r, vec2i pos, vec2i size) {
    EXPECT_EQ(r.pos(), pos);
    EXPECT_EQ(r.size(), size);
}
}  // namespace

TEST(Rectf, DefaultConstructsToZero) {
    Rectf r{};
    expectRectNear(r, {0.0F, 0.0F}, {0.0F, 0.0F});
    EXPECT_FLOAT_EQ(r.left(), 0.0F);
    EXPECT_FLOAT_EQ(r.top(), 0.0F);
    EXPECT_FLOAT_EQ(r.right(), 0.0F);
    EXPECT_FLOAT_EQ(r.bottom(), 0.0F);
    EXPECT_FLOAT_EQ(r.width(), 0.0F);
    EXPECT_FLOAT_EQ(r.height(), 0.0F);
}

TEST(Rectf, ConstructFromComponentsNormalizesPositiveWidthHeight) {
    Rectf r(1.0F, 2.0F, 3.0F, 4.0F);
    expectRectNear(r, {1.0F, 2.0F}, {3.0F, 4.0F});
}

TEST(Rectf, ConstructWithNegativeSizeNormalizesAndShiftsPos) {
    Rectf r(10.0F, 20.0F, -5.0F, -8.0F);
    expectRectNear(r, {5.0F, 12.0F}, {5.0F, 8.0F});
}

TEST(Rectf, ConstructFromPosAndExtent) {
    Rectf r({3.0F, 4.0F}, {7.0F, 9.0F});
    expectRectNear(r, {3.0F, 4.0F}, {7.0F, 9.0F});
}

TEST(Recti, EqualityOnlyForIntegers) {
    Recti a(1, 2, 3, 4);
    Recti b(1, 2, 3, 4);
    Recti c(1, 2, 3, 5);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

TEST(Rectf, SetPositionAndSize) {
    Rectf r{};
    r.setPos(2.0F, 3.0F);
    r.setSize(10.0F, 20.0F);
    expectRectNear(r, {2.0F, 3.0F}, {10.0F, 20.0F});
}

TEST(Rectf, SetWidthHeightNormalizesIfNegative) {
    Rectf r(10.0F, 20.0F, 5.0F, 6.0F);
    r.setWidth(-2.0F);
    expectRectNear(r, {8.0F, 20.0F}, {2.0F, 6.0F});

    r.setHeight(-3.0F);
    expectRectNear(r, {8.0F, 17.0F}, {2.0F, 3.0F});
}

TEST(Rectf, SetLeftMovesPosAndKeepsRightFixed) {
    Rectf r(0.0F, 0.0F, 10.0F, 10.0F);
    r.setLeft(12.0F);
    EXPECT_FLOAT_EQ(r.left(), 10.0F);
    EXPECT_FLOAT_EQ(r.right(), 12.0F);
    EXPECT_FLOAT_EQ(r.width(), 2.0F);
    EXPECT_FLOAT_EQ(r.top(), 0.0F);
    EXPECT_FLOAT_EQ(r.bottom(), 10.0F);
}

TEST(Rectf, SetRightChangesWidthKeepsLeftFixed) {
    Rectf r(5.0F, 7.0F, 3.0F, 4.0F);  // left=5 right=8
    r.setRight(2.0F);
    EXPECT_FLOAT_EQ(r.left(), 2.0F);
    EXPECT_FLOAT_EQ(r.right(), 5.0F);
    EXPECT_FLOAT_EQ(r.width(), 3.0F);
    EXPECT_FLOAT_EQ(r.top(), 7.0F);
    EXPECT_FLOAT_EQ(r.height(), 4.0F);
}

TEST(Rectf, SetTopMovesPosYAndKeepsBottomFixed) {
    Rectf r(0.0F, 0.0F, 10.0F, 10.0F);  // bottom=10
    r.setTop(12.0F);
    EXPECT_FLOAT_EQ(r.top(), 10.0F);
    EXPECT_FLOAT_EQ(r.bottom(), 12.0F);
    EXPECT_FLOAT_EQ(r.height(), 2.0F);
}

TEST(Rectf, SetBottomChangesHeightKeepsTopFixed) {
    Rectf r(1.0F, 2.0F, 3.0F, 4.0F);  // top=2 bottom=6
    r.setBottom(1.0F);
    EXPECT_FLOAT_EQ(r.top(), 1.0F);
    EXPECT_FLOAT_EQ(r.bottom(), 2.0F);
    EXPECT_FLOAT_EQ(r.height(), 1.0F);
}

TEST(Rectf, SetOverloadsSetAndSetWH) {
    Rectf r{};
    r.set(3.0F, 4.0F, 5.0F, 6.0F);
    expectRectNear(r, {3.0F, 4.0F}, {5.0F, 6.0F});
    r.setWH(7.0F, 8.0F);
    expectRectNear(r, {3.0F, 4.0F}, {7.0F, 8.0F});
}

TEST(Rectf, MinMaxWidthHeightAndTBLEdges) {
    Rectf r(2.0F, 3.0F, 5.0F, 7.0F);
    expectVec2Near(r.min(), {2.0F, 3.0F});
    expectVec2Near(r.max(), {7.0F, 10.0F});
    EXPECT_FLOAT_EQ(r.width(), 5.0F);
    EXPECT_FLOAT_EQ(r.height(), 7.0F);

    auto tblr = r.asTBLR();
    EXPECT_FLOAT_EQ(tblr[0], r.top());
    EXPECT_FLOAT_EQ(tblr[1], r.bottom());
    EXPECT_FLOAT_EQ(tblr[2], r.left());
    EXPECT_FLOAT_EQ(tblr[3], r.right());
}

TEST(Rectf, CenterLocalCoordsClampPoint) {
    Rectf r(10.0F, 20.0F, 8.0F, 12.0F);
    expectVec2Near(r.center(), {14.0F, 26.0F});

    expectVec2Near(r.localCoords({10.0F, 20.0F}), {0.0F, 0.0F});
    expectVec2Near(r.localCoords({18.0F, 32.0F}), {1.0F, 1.0F});
    expectVec2Near(r.localCoords({14.0F, 26.0F}), {0.5F, 0.5F});

    expectVec2Near(r.clampPoint({9.0F, 50.0F}), {0.0F, 1.0F});
    expectVec2Near(r.clampPoint({19.0F, 19.0F}), {1.0F, 0.0F});
}

TEST(Rectf, MoveTranslatesPositionOnly) {
    Rectf r(1.0F, 2.0F, 3.0F, 4.0F);
    r.move({-1.0F, +5.0F});
    expectRectNear(r, {0.0F, 7.0F}, {3.0F, 4.0F});
}

TEST(Rectf, ClampedToBounds) {
    Rectf r(10.0F, 10.0F, 10.0F, 10.0F);
    Rectf bounds(15.0F, 5.0F, 6.0F, 12.0F);  // bounds: [15,21]x[5,17]

    Rectf c = r.clampedTo(bounds);
    EXPECT_LE(c.left(), c.right());
    EXPECT_LE(c.top(), c.bottom());
    EXPECT_GE(c.left(), bounds.left());
    EXPECT_LE(c.right(), bounds.right());
    EXPECT_GE(c.top(), bounds.top());
    EXPECT_LE(c.bottom(), bounds.bottom());
}

TEST(Rectf, RelativeToProducesMappingRect) {
    Rectf a(10.0F, 20.0F, 10.0F, 10.0F);
    Rectf b(15.0F, 25.0F, 20.0F, 5.0F);
    Rectf rel = a.relativeTo(b);
    expectRectNear(rel, {(b.left() - a.left()) / a.width(), (b.top() - a.top()) / a.height()}, {b.width() / a.width(), b.height() / a.height()});
}

TEST(Rectf, FromPointsBuildsTightAabb) {
    std::array<vec2f, 4> pts = {vec2f{3.0F, 5.0F}, vec2f{-2.0F, 7.0F}, vec2f{4.0F, 1.0F}, vec2f{0.0F, 9.0F}};
    Rectf r = Rectf::fromPoints(pts);
    expectRectNear(r, {-2.0F, 1.0F}, {6.0F, 8.0F});
}

TEST(Rectf, ExpandWithPoints) {
    Rectf r(0.0F, 0.0F, 2.0F, 2.0F);
    std::array<vec2f, 3> pts = {vec2f{-1.0F, 1.0F}, vec2f{3.0F, -1.0F}, vec2f{1.0F, 4.0F}};
    r.expand(pts);
    expectRectNear(r, {-1.0F, -1.0F}, {4.0F, 5.0F});
}

TEST(Rectf, ExpandWithRect) {
    Rectf a(0.0F, 0.0F, 2.0F, 2.0F);
    Rectf b(1.0F, -1.0F, 5.0F, 4.0F);
    a.expand(b);
    expectRectNear(a, {0.0F, -1.0F}, {6.0F, 4.0F});
}

TEST(Rectf, ExpandWithSinglePoint) {
    Rectf r(1.0F, 1.0F, 2.0F, 2.0F);
    r.expand(vec2f{-1.0F, 3.0F});
    expectRectNear(r, {-1.0F, 1.0F}, {4.0F, 2.0F});
}

TEST(Rectf, ExpandWithEmptySpanNoChange) {
    Rectf r(1.0F, 2.0F, 3.0F, 4.0F);
    std::span<const vec2f> empty{};
    r.expand(empty);
    expectRectNear(r, {1.0F, 2.0F}, {3.0F, 4.0F});
}

TEST(Rectf, AsI32TruncatesTowardZeroPerAs) {
    Rectf r(1.9F, -2.1F, 3.8F, 4.2F);
    Recti ri = r.asI32();
    expectRectEq(ri, {as<i32>(1.9F), as<i32>(-2.1F)}, {as<i32>(3.8F), as<i32>(4.2F)});
}

TEST(Recti, AsF32CastsComponents) {
    Recti ri(1, 2, 3, 4);
    Rectf rf = ri.asF32();
    expectRectNear(rf, {1.0F, 2.0F}, {3.0F, 4.0F});
}

TEST(Rectf, AlmostEqualUsesEpsilon) {
    Rectf a(1.0F, 2.0F, 3.0F, 4.0F);

    Rectf b(1.0F + 1e-7F, 2.0F - 1e-7F, 3.0F + 5e-7F, 4.0F - 5e-7F);
    EXPECT_TRUE(a.almostEqual(b, 1e-6F));
    Rectf c(1.0F + 2e-5F, 2.0F - 3e-5F, 3.0F + 2e-5F, 4.0F - 3e-5F);
    EXPECT_FALSE(a.almostEqual(c, 1e-6F));
}

TEST(Line2D, DefaultConstructsRightFromOrigin) {
    Line2D l;
    expectVec2Near(l.origin(), {0.0F, 0.0F});
    expectVec2Near(l.direction(), {1.0F, 0.0F});
}

TEST(Line2D, FromPointsBuildsNormalizedDirection) {
    auto l = Line2D::fromPoints({0.0F, 0.0F}, {0.0F, 2.0F});
    expectVec2Near(l.origin(), {0.0F, 0.0F});
    // dir should be (0,1)
    expectVec2Near(l.direction(), {0.0F, 1.0F});
}

TEST(Line2D, AtProjectsPointAlongDirection) {
    Line2D l({1.0F, 2.0F}, {0.0F, 1.0F});
    expectVec2Near(l.at(3.0F), {1.0F, 5.0F});
}

TEST(Line2D, ProjectAndClosestPointAgree) {
    Line2D l({1.0F, 1.0F}, glm::normalize(vec2f{1.0F, 1.0F}));
    vec2f p{3.0F, 1.0F};
    const f32 t = l.project(p);
    expectVec2Near(l.closestPoint(p), l.at(t));
}

TEST(Line2D, ContainsReturnsTrueForPointsOnLineWithinEps) {
    Line2D l({0.0F, 0.0F}, {1.0F, 0.0F});
    EXPECT_TRUE(l.intersect(vec2f{5.0F, 0.0F}, 1e-6F));
    EXPECT_FALSE(l.intersect(vec2f{5.0F, 1e-3F}, 1e-6F));
}

TEST(LineSegment2D, DefaultConstructsZeroSegment) {
    LineSegment2D s;
    expectVec2Near(s.getA(), {0.0F, 0.0F});
    expectVec2Near(s.getB(), {0.0F, 0.0F});
    EXPECT_FLOAT_EQ(s.getLength(), 0.0F);
}

TEST(LineSegment2D, DirectionLengthCenter) {
    LineSegment2D s({1.0F, 2.0F}, {5.0F, 5.0F});
    expectVec2Near(s.getDirection(), {4.0F, 3.0F});
    EXPECT_NEAR(s.getLength(), glm::length(vec2f{4.0F, 3.0F}), 1e-6F);
    expectVec2Near(s.getCenter(), {3.0F, 3.5F});
}

TEST(LineSegment2D, AtInterpolates) {
    LineSegment2D s({0.0F, 0.0F}, {4.0F, 0.0F});
    expectVec2Near(s.at(0.0F), {0.0F, 0.0F});
    expectVec2Near(s.at(1.0F), {4.0F, 0.0F});
    expectVec2Near(s.at(0.25F), {1.0F, 0.0F});
}

TEST(LineSegment2D, ClosestPointClampsToSegment) {
    LineSegment2D s({0.0F, 0.0F}, {4.0F, 0.0F});
    expectVec2Near(s.closestPoint({-2.0F, 1.0F}), {0.0F, 0.0F});
    expectVec2Near(s.closestPoint({10.0F, -3.0F}), {4.0F, 0.0F});
    expectVec2Near(s.closestPoint({1.0F, 3.0F}), {1.0F, 0.0F});
}

TEST(LineSegment2D, ContainsWithEpsilon) {
    LineSegment2D s({0.0F, 0.0F}, {4.0F, 0.0F});
    EXPECT_TRUE(s.intersect(vec2f{2.0F, 0.0F}, 1e-6F));
    EXPECT_TRUE(s.intersect(vec2f{0.0F, 0.0F}, 1e-6F));
    EXPECT_TRUE(s.intersect(vec2f{4.0F, 0.0F}, 1e-6F));
    EXPECT_FALSE(s.intersect(vec2f{2.0F, 1e-3F}, 1e-6F));
}

TEST(Ray2D, DefaultConstructsRightFromOrigin) {
    Ray2D r;
    expectVec2Near(r.getOrigin(), {0.0F, 0.0F});
    expectVec2Near(r.getDirection(), {1.0F, 0.0F});
}

TEST(Ray2D, FromPointsUsesNormalizedDirection) {
    auto r = Ray2D::fromPoints({1.0F, 2.0F}, {1.0F, 6.0F});
    expectVec2Near(r.getOrigin(), {1.0F, 2.0F});
    expectVec2Near(r.getDirection(), {0.0F, 1.0F});
}

TEST(Ray2D, AtTranslatesAlongDirectionForTGeZero) {
    Ray2D r({1.0F, 1.0F}, {0.0F, 1.0F});
    expectVec2Near(r.at(3.0F), {1.0F, 4.0F});
}

TEST(Ray2D, ProjectIsClampedToNonNegative) {
    Ray2D r({1.0F, 1.0F}, {1.0F, 0.0F});
    EXPECT_FLOAT_EQ(r.project({0.0F, 1.0F}), 0.0F);
    EXPECT_FLOAT_EQ(r.project({4.0F, 1.0F}), 3.0F);
}

TEST(Ray2D, GetClosestPointClampsUsingProject) {
    Ray2D r({1.0F, 1.0F}, {1.0F, 0.0F});
    expectVec2Near(r.getClosestPoint({0.0F, 5.0F}), {1.0F, 1.0F});
    expectVec2Near(r.getClosestPoint({4.0F, 2.0F}), {4.0F, 1.0F});
}
TEST(Circle, DefaultConstructsZero) {
    Circle c;
    expectVec2Near(c.center(), {0.0F, 0.0F});
}

TEST(Circle, ConstructAndCacheRadiusSquared) {
    Circle c({2.0F, -1.0F}, 3.0F);
    expectVec2Near(c.center(), {2.0F, -1.0F});
    EXPECT_FLOAT_EQ(c.radius(), 3.0F);
    EXPECT_FLOAT_EQ(c.radius2(), 9.0F);
}

TEST(Circle, SettersUpdateCenterAndRadius2) {
    Circle c;
    c.setCenter({1.0F, 2.0F});
    c.setRadius(2.5F);
    expectVec2Near(c.center(), {1.0F, 2.0F});
    EXPECT_FLOAT_EQ(c.radius(), 2.5F);
    EXPECT_FLOAT_EQ(c.radius2(), 6.25F);
}

TEST(Circle, SignedDistancePositiveOutsideNegativeInsideZeroAtBoundary) {
    Circle c({0.0F, 0.0F}, 2.0F);
    EXPECT_NEAR(c.signedDistance({3.0F, 0.0F}), 1.0F, 1e-6F);   // outside
    EXPECT_NEAR(c.signedDistance({1.0F, 0.0F}), -1.0F, 1e-6F);  // inside
    EXPECT_NEAR(c.signedDistance({2.0F, 0.0F}), 0.0F, 1e-6F);   // on boundary
}

TEST(Circle, ContainsUsesDistanceSquared) {
    Circle c({0.0F, 0.0F}, 2.0F);
    EXPECT_TRUE(c.intersect(vec2f{1.0F, 1.0F}));
    EXPECT_TRUE(c.intersect(vec2f{2.0F, 0.0F}));
    EXPECT_FALSE(c.intersect(vec2f{2.001F, 0.0F}));
}

TEST(Circle, ClosestPointToExternalAndInternalPoints) {
    Circle c({1.0F, 1.0F}, 2.0F);
    vec2f p_out{5.0F, 1.0F};
    vec2f v = glm::normalize(p_out - c.center());
    expectVec2Near(c.closestPoint(p_out), c.center() + v * c.radius(), 1e-5F);

    vec2f p_in{2.0F, 1.0F};
    v = glm::normalize(p_in - c.center());
    expectVec2Near(c.closestPoint(p_in), c.center() + v * c.radius(), 1e-5F);
}
