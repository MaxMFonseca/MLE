#include <gtest/gtest.h>

#include "mle/math/Types3D.h"
#include "mle/renderer/Camera.h"

using namespace mle;  // NOLINT

namespace {
constexpr f32 K_EPS = 1e-4F;
bool almostEqual(f32 a, f32 b, f32 eps = K_EPS) {
    return std::abs(a - b) <= eps;
}

::testing::AssertionResult matNear(const mat4f& A, const mat4f& B, f32 eps = K_EPS) {
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            if (!almostEqual(A[c][r], B[c][r], eps)) {
                return ::testing::AssertionFailure() << "Matrices differ at [" << r << "," << c << "]: " << A[c][r] << " vs " << B[c][r] << " (eps " << eps
                                                     << ")";
            }
        }
    }
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult vecNear(const vec3f& a, const vec3f& b, f32 eps = K_EPS) {
    if (!almostEqual(a.x, b.x, eps) || !almostEqual(a.y, b.y, eps) || !almostEqual(a.z, b.z, eps)) {
        return ::testing::AssertionFailure() << "Vectors differ: (" << a.x << "," << a.y << "," << a.z << ") vs (" << b.x << "," << b.y << "," << b.z
                                             << ") (eps " << eps << ")";
    }
    return ::testing::AssertionSuccess();
}

mat4f identity() {
    return {1.0F};
}
}  // namespace

TEST(Camera, DefaultsProduceExpectedViewAndPerspectiveProj) {
    Camera cam;

    EXPECT_EQ(cam.getProjType(), Camera::ProjType::PERSPECTIVE);
    EXPECT_TRUE(vecNear(cam.getEye(), vec3f(0.0F, 0.0F, 5.0F)));
    EXPECT_TRUE(vecNear(cam.getTarget(), vec3f(0.0F, 0.0F, 0.0F)));
    EXPECT_TRUE(vecNear(cam.getUp(), vec3f(0.0F, 1.0F, 0.0F)));

    EXPECT_TRUE(almostEqual(cam.getFov(), 60.0F));
    EXPECT_TRUE(almostEqual(cam.getAspect(), 16.0F / 9.0F));
    EXPECT_TRUE(almostEqual(cam.getNear(), 0.1F));
    EXPECT_TRUE(almostEqual(cam.getFar(), 1000.0F));

    mat4f expected_view = glm::lookAt(cam.getEye(), cam.getTarget(), cam.getUp());
    EXPECT_TRUE(matNear(cam.getView(), expected_view));

    mat4f expected_proj = glm::perspective(glm::radians(cam.getFov()), cam.getAspect(), cam.getNear(), cam.getFar());
    expected_proj[1][1] = -expected_proj[1][1];
    EXPECT_TRUE(matNear(cam.getProj(), expected_proj));

    EXPECT_TRUE(matNear(cam.getViewProj(), expected_proj * expected_view));
}

TEST(Camera, ViewAndProjHaveValidInverses) {
    Camera cam;
    mat4f v = cam.getView();
    mat4f p = cam.getProj();
    mat4f vp = cam.getViewProj();

    mat4f vi = cam.getInvView();
    mat4f pi = cam.getInvProj();
    mat4f v_pi = cam.getInvViewProj();

    EXPECT_TRUE(matNear(v * vi, identity(), 1e-3F));
    EXPECT_TRUE(matNear(p * pi, identity(), 1e-3F));
    EXPECT_TRUE(matNear(vp * v_pi, identity(), 1e-3F));
}

TEST(Camera, LookAtDirSetsTargetRelativeToEye) {
    Camera cam;
    cam.setEye(vec3f(1, 2, 3));
    cam.lookAtDir(vec3f(0, 0, -5));
    vec3f forward = glm::normalize(vec3f(0, 0, -5));
    EXPECT_TRUE(vecNear(cam.getTarget(), cam.getEye() + forward));
}

TEST(Camera, WalkTranslatesEyeAndTargetEqually) {
    Camera cam;
    vec3f old_eye = cam.getEye();
    vec3f old_tgt = cam.getTarget();

    vec3f delta(2.0F, -1.0F, 0.5F);
    cam.walk(delta);

    EXPECT_TRUE(vecNear(cam.getEye(), old_eye + delta));
    EXPECT_TRUE(vecNear(cam.getTarget(), old_tgt + delta));

    mat4f expected_view = glm::lookAt(cam.getEye(), cam.getTarget(), cam.getUp());
    EXPECT_TRUE(matNear(cam.getView(), expected_view));
}

TEST(Camera, MoveIsLocalSpaceTranslation) {
    Camera cam;
    vec3f old_eye = cam.getEye();
    vec3f old_tgt = cam.getTarget();

    cam.move(vec3f(0, 0, +1));
    EXPECT_LT(cam.getEye().z, old_eye.z);
    EXPECT_LT(cam.getTarget().z, old_tgt.z);
}

TEST(Camera, OrbitKeepsRadiusAndClampsPitch) {
    Camera cam;
    const f32 initial_radius = glm::length(cam.getEye() - cam.getTarget());

    cam.rotateAroundTarget(glm::half_pi<f32>(), glm::pi<f32>());
    const f32 new_radius = glm::length(cam.getEye() - cam.getTarget());

    EXPECT_TRUE(almostEqual(initial_radius, new_radius, 1e-3F));

    auto v = cam.getView();
    EXPECT_TRUE(std::isfinite(glm::determinant(v)));
}

TEST(Camera, ZoomClampsDistance) {
    Camera cam;
    cam.zoom(100.0F);
    f32 dist = glm::length(cam.getEye() - cam.getTarget());
    EXPECT_GE(dist, 0.1F - 1e-5F);
    EXPECT_LE(dist, 0.1002F);
}

TEST(Camera, SwitchingToOrthographicAndRectMatchesGlmOrthoWithYFlip) {
    Camera cam;
    cam.setProjType(Camera::ProjType::ORTH);
    cam.setNear(0.5F);
    cam.setFar(500.0F);
    cam.setRect(-2.0F, 2.0F, -1.0F, 3.0F);

    mat4f expected = glm::ortho(-2.0F, 2.0F, -1.0F, 3.0F, 0.5F, 500.0F);
    expected[1][1] = -expected[1][1];

    EXPECT_TRUE(matNear(cam.getProj(), expected));
}

TEST(Camera, AspectAndFovUpdatePerspectiveMatrix) {
    Camera cam;
    cam.setProjType(Camera::ProjType::PERSPECTIVE);

    cam.setFov(75.0F);
    cam.setAspect(21.0F / 9.0F);
    cam.setNear(0.05F);
    cam.setFar(250.0F);

    mat4f expected = glm::perspective(glm::radians(75.0F), 21.0F / 9.0F, 0.05F, 250.0F);
    expected[1][1] = -expected[1][1];

    EXPECT_TRUE(matNear(cam.getProj(), expected));
}

namespace {
struct ClusterGrid {
    u32 x_tiles{};
    u32 y_tiles{};
    u32 z_slices{};
};

ClusterGrid gridFor(u32 tile_px, u32 w, u32 h, u32 z) {
    const u32 xt = (w + tile_px - 1) / tile_px;
    const u32 yt = (h + tile_px - 1) / tile_px;
    return {.x_tiles = xt, .y_tiles = yt, .z_slices = z};
}

size_t clusterIndex(u32 x, u32 y, u32 z, const ClusterGrid& g) {
    return as<usize>((((z * g.y_tiles) + y) * g.x_tiles) + x);
}

struct Extents {
    f32 half_width;
    f32 half_height;
};

Extents expectedExtentsAtDepth(const Camera& cam, float dAbs) {
    const f32 fov_rad = glm::radians(cam.getFov());
    const f32 tan_half = std::tan(fov_rad * 0.5F);
    Extents e{};
    e.half_height = dAbs * tan_half;
    e.half_width = e.half_height * cam.getAspect();
    return e;
}
}  // namespace

TEST(Camera, ComputeViewClustersReturnsExpectedCount) {
    Camera cam;
    const u32 tile = 64;
    const u32 w = 128;
    const u32 h = 128;
    const u32 z = 16;

    const auto clusters = cam.computeViewClusters(tile, w, h, z);
    ASSERT_EQ(clusters.size(), as<usize>(2 * 2 * z));
}

TEST(Camera, CountScalesWithTilesAndZSlices) {
    Camera cam;
    const auto a = cam.computeViewClusters(64, 128, 128, 8);  // 2x2x8 = 32
    const auto b = cam.computeViewClusters(32, 128, 128, 8);  // 4x4x8 = 128
    const auto c = cam.computeViewClusters(64, 256, 64, 16);  // 4x1x16 = 64

    EXPECT_EQ(a.size(), as<usize>(2 * 2 * 8));
    EXPECT_EQ(b.size(), as<usize>(4 * 4 * 8));
    EXPECT_EQ(c.size(), as<usize>(4 * 1 * 16));
}

TEST(Camera, ExtentsWidenMonotonicallyWithDepthForSameTile) {
    Camera cam;
    const u32 tile = 64, w = 256, h = 256, z = 12;
    const auto clusters = cam.computeViewClusters(tile, w, h, z);
    const ClusterGrid g = gridFor(tile, w, h, z);

    const u32 xi = g.x_tiles / 2;
    const u32 yi = g.y_tiles / 2;

    f32 prev_half_span_x = 0.0F;
    f32 prev_half_span_y = 0.0F;

    for (u32 zi = 0; zi < g.z_slices; ++zi) {
        const auto& box = clusters[clusterIndex(xi, yi, zi, g)];
        const auto mn = box.min();
        const auto mx = box.max();

        const f32 half_span_x = 0.5F * (mx.x - mn.x);
        const f32 half_span_y = 0.5F * (mx.y - mn.y);

        if (zi > 0) {
            EXPECT_GE(half_span_x + 1e-5F, prev_half_span_x);
            EXPECT_GE(half_span_y + 1e-5F, prev_half_span_y);
        }
        prev_half_span_x = half_span_x;
        prev_half_span_y = half_span_y;
    }
}

TEST(Camera, AdjacentTilesAreOrderedLeftToRightForSameRowAndSlice) {
    Camera cam;
    const u32 tile = 64, w = 256, h = 128, z = 4;
    const auto clusters = cam.computeViewClusters(tile, w, h, z);
    const ClusterGrid g = gridFor(tile, w, h, z);

    const u32 yi = g.y_tiles / 2;
    const u32 zi = g.z_slices / 2;

    f32 prev_min_x = -mle::inf<f32>();
    for (u32 xi = 0; xi < g.x_tiles; ++xi) {
        const auto& box = clusters[clusterIndex(xi, yi, zi, g)];
        const auto mn = box.min();
        EXPECT_GE(mn.x + 1e-6F, prev_min_x);
        prev_min_x = mn.x;
    }
}

TEST(Camera, LargerFovOrAspectProducesWiderExtentsAtSameDepth) {
    Camera cam_a;
    Camera cam_b = cam_a;
    Camera cam_c = cam_a;

    cam_b.setFov(90.0F);

    cam_c.setAspect(21.0F / 9.0F);

    const u32 tile = 64, w = 256, h = 256, z = 10;
    const ClusterGrid g = gridFor(tile, w, h, z);
    const u32 xi = g.x_tiles / 2, yi = g.y_tiles / 2, zi = g.z_slices - 1;

    const auto a = cam_a.computeViewClusters(tile, w, h, z)[clusterIndex(xi, yi, zi, g)];
    const auto b = cam_b.computeViewClusters(tile, w, h, z)[clusterIndex(xi, yi, zi, g)];
    const auto c = cam_c.computeViewClusters(tile, w, h, z)[clusterIndex(xi, yi, zi, g)];

    const auto amin = a.min(), amax = a.max();
    const auto bmin = b.min(), bmax = b.max();
    const auto cmin = c.min(), cmax = c.max();

    const float a_half_x = 0.5F * (amax.x - amin.x);
    const float b_half_x = 0.5F * (bmax.x - bmin.x);
    const float c_half_x = 0.5F * (cmax.x - cmin.x);

    EXPECT_GT(b_half_x, a_half_x);

    EXPECT_GT(c_half_x, a_half_x);
}

TEST(Camera, FarSliceMaxMatchesAnalyticFrustumWidthWithinTolerance) {
    Camera cam;
    const u32 tile = 64, w = 256, h = 256, z = 8;
    const ClusterGrid g = gridFor(tile, w, h, z);

    const u32 zi = g.z_slices - 1;
    const auto left_box = cam.computeViewClusters(tile, w, h, z)[clusterIndex(0, g.y_tiles / 2, zi, g)];
    const auto right_box = cam.computeViewClusters(tile, w, h, z)[clusterIndex(g.x_tiles - 1, g.y_tiles / 2, zi, g)];

    const auto lmin = left_box.min();
    const auto rmin = right_box.min(), rmax = right_box.max();

    const float d_abs = std::max(std::abs(lmin.z), std::abs(rmin.z));
    const auto ext = expectedExtentsAtDepth(cam, d_abs);

    EXPECT_NEAR(lmin.x, -ext.half_width, 0.15F * ext.half_width);
    EXPECT_NEAR(rmax.x, +ext.half_width, 0.15F * ext.half_width);
}
