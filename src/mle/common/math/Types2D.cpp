#include "Types2D.h"

#include <algorithm>

#include "mle/common/Logger.h"

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

bool isPointInsidePolygon(vec2f p, const std::vector<vec2f>& poly) {
    int winding_number = 0;
    usize n = poly.size();
    for (usize i = 0; i < n; ++i) {
        const auto& v0 = poly[i];
        const auto& v1 = poly[(i + 1) % n];

        if (v0.y <= p.y) {
            if (v1.y > p.y && glm::cross(vec3f(v1 - v0, 0.0F), vec3f(p - v0, 0.0F)).z > 0.0F) {
                ++winding_number;
            }
        } else {
            if (v1.y <= p.y && glm::cross(vec3f(v1 - v0, 0.0F), vec3f(p - v0, 0.0F)).z < 0.0F) {
                --winding_number;
            }
        }
    }
    return winding_number != 0;
}

bool isInside(const std::vector<vec2f>& a, const std::vector<vec2f>& b) {
    return std::ranges::all_of(b, [a](const vec2f& pt) { return isPointInsidePolygon(pt, a); });
}
}  // namespace mle
