#include "Types2D.h"

#include <algorithm>

#include "mle/utils/Types.h"

namespace mle {
vec2f LineSegment2D::closestPoint(vec2f point) const {
    vec2f ab = getDirection();
    f32 t = glm::dot(point - a_, ab) / glm::dot(ab, ab);
    t = glm::clamp(t, 0.0F, 1.0F);
    return at(t);
}

bool LineSegment2D::contains(vec2f point, f32 epsilon) const {
    vec2f cp = closestPoint(point);
    return mle::feq(glm::distance2(cp, point), 0.0F, epsilon * epsilon);
}

bool Line2D::contains(vec2f point, f32 epsilon) const {
    vec2f cp = closestPoint(point);
    return mle::feq(glm::distance2(cp, point), 0.0F, epsilon * epsilon);
}

f32 Polygon2f::area() const {
    f32 a = 0.0F;
    const usize n = verts_.size();
    for (usize i = 0; i < n; ++i) {
        const auto& p0 = verts_[i];
        const auto& p1 = verts_[(i + 1) % n];
        a += (p0.x * p1.y - p1.x * p0.y);
    }
    return a * 0.5F;
}

vec2f Polygon2f::center() const {
    vec2f c{0.0F};
    for (const auto& p : verts_) {
        c += p;
    }
    return c / static_cast<f32>(verts_.size());
}

bool Polygon2f::contains(vec2f p) const {
    bool inside = false;
    const usize n = verts_.size();
    for (usize i = 0, j = n - 1; i < n; j = i++) {
        const auto& vi = verts_[i];
        const auto& vj = verts_[j];
        if (((vi.y > p.y) != (vj.y > p.y)) && (p.x < (vj.x - vi.x) * (p.y - vi.y) / (vj.y - vi.y + 1e-6F) + vi.x)) {
            inside = !inside;
        }
    }
    return inside;
}

void Polygon2f::sortCCW() {
    const vec2f c = center();
    std::ranges::sort(verts_, [c](const vec2f& a, const vec2f& b) { return std::atan2(a.y - c.y, a.x - c.x) < std::atan2(b.y - c.y, b.x - c.x); });
}

vec2f Circle::closestPoint(vec2f point) const {
    const vec2f delta = point - center_;
    const f32 dist = glm::length(delta);
    if (dist <= radius_ || dist == 0.F) {
        return center_ + vec2f(radius_, 0.F);
    }
    return center_ + delta * (radius_ / dist);
}

bool Circle::intersects(const Circle& other) const {
    const f32 r_sum = radius_ + other.radius_;
    return glm::distance2(center_, other.center_) <= r_sum * r_sum;
}

AABB2D::AABB2D(std::span<const vec2f> points) {
    min_ = vec2f{inf<f32>()};
    max_ = vec2f{-inf<f32>()};
    for (const auto& p : points) {
        expand(p);
    }
}

void AABB2D::expand(vec2f p) {
    min_.x = std::min(min_.x, p.x);
    min_.y = std::min(min_.y, p.y);
    max_.x = std::max(max_.x, p.x);
    max_.y = std::max(max_.y, p.y);
}
}  // namespace mle
