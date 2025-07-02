#include "Types2D.h"

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
}  // namespace mle
