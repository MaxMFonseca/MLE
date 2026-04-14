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

vec2f Circle::closestPoint(vec2f point) const {
    const vec2f delta = point - center_;
    const f32 dist = glm::length(delta);
    if (dist <= radius_ || dist == 0.F) {
        return center_ + vec2f(radius_, 0.F);
    }
    return center_ + delta * (radius_ / dist);
}
}  // namespace mle
