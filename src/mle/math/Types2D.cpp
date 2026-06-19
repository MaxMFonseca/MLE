#include "Types2D.h"

#include <algorithm>

#include "mle/math/Utils.h"
#include "mle/utils/Types.h"

namespace {
// NOLINTNEXTLINE(misc-no-recursion) cool recursion
void rdpImpl(const std::vector<mle::vec2f>& pts, mle::usize start, mle::usize end, mle::f32 epsilon2, std::vector<bool>& keep) {
    if (end <= start + 1) {
        return;
    }

    const mle::vec2f& a = pts[start];
    const mle::vec2f& b = pts[end];
    const mle::vec2f ab = b - a;
    const mle::f32 ab_len2 = glm::dot(ab, ab);

    mle::f32 max_dist2 = 0.0F;
    mle::usize max_idx = start + 1;

    for (mle::usize i = start + 1; i < end; ++i) {
        mle::f32 dist2 = 0.0F;
        if (ab_len2 < mle::FloatTolerance<mle::f32>::ABS * mle::FloatTolerance<mle::f32>::ABS) {
            const mle::vec2f d = pts[i] - a;
            dist2 = glm::dot(d, d);
        } else {
            const mle::vec2f ap = pts[i] - a;
            const mle::f32 t = glm::dot(ap, ab) / ab_len2;
            const mle::vec2f proj = a + t * ab;
            const mle::vec2f diff = pts[i] - proj;
            dist2 = glm::dot(diff, diff);
        }
        if (dist2 > max_dist2) {
            max_dist2 = dist2;
            max_idx = i;
        }
    }

    if (max_dist2 > epsilon2) {
        keep[max_idx] = true;
        rdpImpl(pts, start, max_idx, epsilon2, keep);
        rdpImpl(pts, max_idx, end, epsilon2, keep);
    }
}
}  // anonymous namespace

namespace mle {
vec2f LineSegment2D::closestPoint(vec2f point) const {
    vec2f ab = getDirection();
    f32 t = glm::dot(point - a_, ab) / glm::dot(ab, ab);
    t = glm::clamp(t, 0.0F, 1.0F);
    return at(t);
}

f32 Polygon2f::area() const {
    assert(false && "unimplemented");
    return 0;
}

vec2f Polygon2f::center() const {
    assert(false && "unimplemented");
    return {};
}

void Polygon2f::sortCCW() {
    assert(false && "unimplemented");
}

void Polygon2f::simplify(f32 epsilon) {
    if (epsilon <= 0.0F || verts_.size() < 3) {
        return;
    }

    const usize n = verts_.size();
    std::vector<bool> keep(n, false);
    keep[0] = true;
    keep[n - 1] = true;

    rdpImpl(verts_, 0, n - 1, epsilon * epsilon, keep);

    std::vector<vec2f> result;
    result.reserve(n);
    for (usize i = 0; i < n; ++i) {
        if (keep[i]) {
            result.push_back(verts_[i]);
        }
    }
    verts_ = std::move(result);
}

vec2f Circle::closestPoint(vec2f point) const {
    const vec2f delta = point - center_;
    const f32 dist = glm::length(delta);
    if (dist <= radius_ || dist == 0.F) {
        return center_ + vec2f(radius_, 0.F);
    }
    return center_ + delta * (radius_ / dist);
}
}  // namespace mle
