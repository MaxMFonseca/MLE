#pragma once

#include "mle/math/Types.h"
#include "mle/math/Utils.h"

namespace mle {
template <typename T>
class Rect;
using Rectf = Rect<f32>;
class Circle;
class Ray2D;
class Line2D;
class LineSegment2D;
class Polygon2f;

struct Intersect2D {
    /// Rectf vs point
    [[nodiscard]] static bool intersect(const Rectf& r, vec2f p, f32 epsilon = FloatTolerance<f32>::REL);
    /// point vs Rectf
    [[nodiscard]] static bool intersect(vec2f p, const Rectf& r, f32 epsilon = FloatTolerance<f32>::REL) { return intersect(r, p, epsilon); }

    /// Circle vs point.
    [[nodiscard]] static bool intersect(const Circle& c, vec2f p, f32 epsilon = FloatTolerance<f32>::REL);
    /// point vs Circle
    [[nodiscard]] static bool intersect(vec2f p, const Circle& c, f32 epsilon = FloatTolerance<f32>::REL) { return intersect(c, p, epsilon); }

    /// Line2D vs point.
    [[nodiscard]] static bool intersect(const Line2D& l, vec2f p, f32 epsilon = FloatTolerance<f32>::REL);
    /// point vs Line2D
    [[nodiscard]] static bool intersect(vec2f p, const Line2D& l, f32 epsilon = FloatTolerance<f32>::REL) { return intersect(l, p, epsilon); }

    /// Ray2D vs point.
    [[nodiscard]] static bool intersect(const Ray2D& r, vec2f p, f32 epsilon = FloatTolerance<f32>::REL);
    /// point vs Ray2D
    [[nodiscard]] static bool intersect(vec2f p, const Ray2D& r, f32 epsilon = FloatTolerance<f32>::REL) { return intersect(r, p, epsilon); }

    /// LineSegment2D vs point.
    [[nodiscard]] static bool intersect(const LineSegment2D& s, vec2f p, f32 epsilon = FloatTolerance<f32>::REL);
    /// point vs LineSegment2D
    [[nodiscard]] static bool intersect(vec2f p, const LineSegment2D& s, f32 epsilon = FloatTolerance<f32>::REL) { return intersect(s, p, epsilon); }

    /// Rectf vs Rectf
    [[nodiscard]] static bool intersect(const Rectf& a, const Rectf& b, f32 epsilon);

  private:
    [[nodiscard]] static constexpr f32 cross2(vec2f a, vec2f b) { return (a.x * b.y) - (a.y * b.x); }
    [[nodiscard]] static constexpr f32 cross2(vec2f a, vec2f b, vec2f c) { return cross2(b - a, c - a); }
    [[nodiscard]] static constexpr bool onSegmentEps(vec2f p, vec2f q, vec2f r, f32 eps) {
        const bool inx = fle(glm::min(p.x, r.x), q.x, eps) && fge(glm::max(p.x, r.x), q.x, eps);
        const bool iny = fle(glm::min(p.y, r.y), q.y, eps) && fge(glm::max(p.y, r.y), q.y, eps);
        return inx && iny;
    }
};
}  // namespace mle
