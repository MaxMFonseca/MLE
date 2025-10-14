#include "Intersect2D.h"

#include "Types2D.h"

namespace mle {
bool Intersect2D::intersect(const Rectf& rect, vec2f point, f32 epsilon) {
    return fge(point.x, rect.left(), epsilon) && fle(point.x, rect.right(), epsilon) && fge(point.y, rect.top(), epsilon) &&
           fle(point.y, rect.bottom(), epsilon);
}

bool Intersect2D::intersect(const Rectf& a, const Rectf& b, f32 epsilon) {
    return fle(a.left(), b.right(), epsilon) && fge(a.right(), b.left(), epsilon) && fle(a.top(), b.bottom(), epsilon) && fge(a.bottom(), b.top(), epsilon);
}

bool Intersect2D::intersect(const Circle& c, vec2f p, f32 epsilon) {
    const f32 rr = (c.radius() + epsilon) * (c.radius() + epsilon);
    return glm::distance2(p, c.center()) <= rr;
}

bool Intersect2D::intersect(const Line2D& l, vec2f p, f32 epsilon) {
    const vec2f v = p - l.origin();
    const f32 dist = std::abs((v.x * l.direction().y) - (v.y * l.direction().x));
    return dist <= epsilon;
}

bool Intersect2D::intersect(const Ray2D& r, vec2f p, f32 epsilon) {
    const vec2f v = p - r.getOrigin();
    const f32 cross = (v.x * r.getDirection().y) - (v.y * r.getDirection().x);
    if (!feq(cross, 0.0F, epsilon)) {
        return false;
    }
    const f32 t = glm::dot(v, r.getDirection());
    return fge(t, 0.0F, epsilon);
}

bool Intersect2D::intersect(const LineSegment2D& s, vec2f p, f32 epsilon) {
    const vec2f a = s.getA();
    const vec2f b = s.getB();
    const vec2f ab = b - a;
    const vec2f ap = p - a;

    const f32 cross = (ap.x * ab.y) - (ap.y * ab.x);
    if (!feq(cross, 0.0F, epsilon)) {
        return false;
    }

    const f32 ab2 = glm::dot(ab, ab);
    if (feq(ab2, 0.0F, epsilon)) {
        return glm::distance2(p, a) <= epsilon * epsilon;
    }

    const f32 t = glm::dot(ap, ab);
    return fge(t, 0.0F, epsilon) && fle(t, ab2, epsilon);
}

}  // namespace mle
