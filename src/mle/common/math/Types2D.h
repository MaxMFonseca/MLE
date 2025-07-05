/**
 * @file Types2D.h
 * @brief 2D geometry primitives and utilities.
 */
#pragma once

#include "Types.h"

namespace mle {
/**
 * @ingroup MathTypes
 * @{
 */
template <typename T>
using Point2 = vec2<T>;       ///< A 2D point represented by a vector of two components.
using Point2u = Point2<u32>;  ///< 2D unsigned integer point.
using Point2i = Point2<i32>;  ///< 2D signed integer point.
using Point2f = Point2<f32>;  ///< 2D floating-point point.
using Point2d = Point2<f64>;  ///< 2D double-precision point.

template <typename T>
using Extent2 = vec2<T>;        ///< A 2D extent represented by a vector of two components.
using Extent2u = Extent2<u32>;  ///< 2D unsigned integer extent.
using Extent2i = Extent2<i32>;  ///< 2D signed integer extent.
using Extent2f = Extent2<f32>;  ///< 2D floating-point extent.
using Extent2d = Extent2<f64>;  ///< 2D double-precision extent.
/// @}

/**
 * @brief Axis-aligned rectangle represented by position and size.
 * @ingroup MathTypes
 *
 * @tparam T Scalar type (e.g., f32, i32)
 */
template <typename T>
struct Rect {
    vec2<T> pos;   ///< Top-left corner of the rectangle.
    vec2<T> size;  ///< The size of the rectangle.

    /// Constructs a ZERO rectangle.
    Rect() = default;

    /// Constructs a rectangle from raw components.
    Rect(T x, T y, T width, T height) :
        pos(x, y),
        size(width, height) {}

    /// Constructs a rectangle from a position and extent
    Rect(vec2<T> pos, Extent2<T> size) :
        pos(pos),
        size(size) {}

    [[nodiscard]] constexpr T left() const { return pos.x; }             ///< Left edge (x position).
    [[nodiscard]] constexpr T right() const { return pos.x + size.x; }   ///< Right edge (x + width).
    [[nodiscard]] constexpr T top() const { return pos.y; }              ///< Top edge (y position).
    [[nodiscard]] constexpr T bottom() const { return pos.y + size.y; }  ///< Bottom edge (y + height).

    [[nodiscard]] constexpr T width() const { return size.x; }   ///< Width of the rectangle.
    [[nodiscard]] constexpr T height() const { return size.y; }  ///< Height of the rectangle.

    [[nodiscard]] constexpr vec2<T> min() const { return pos; }         ///< Minimum corner of the rectangle.
    [[nodiscard]] constexpr vec2<T> max() const { return pos + size; }  ///< Maximum corner of the rectangle.

    [[nodiscard]] constexpr vec2<T> center() const { return pos + (size * 0.5F); }  ///< Center of the rectangle.

    /// True if width and height are greater than 0.
    [[nodiscard]] constexpr bool valid() const { return width() > 0 && height() > 0; }

    /// Translates rectangle by a given offset.
    constexpr void move(vec2<T> offset) { pos += offset; }

    /// True if the rectangles are equal. Only enabled for integer types
    [[nodiscard]] constexpr bool operator==(const Rect<T>& other) const
        requires(!std::is_floating_point_v<T>)
    {
        return pos == other.pos && size == other.size;
    }

    // ---------- Float-only member functions ----------

    /// True if the rectangles are equal within a given epsilon.
    [[nodiscard]] constexpr bool almostEqual(const Rect<T>& rhs, T epsilon) const
        requires std::is_floating_point_v<T>
    {
        return feq(pos.x, rhs.pos.x, epsilon) && feq(pos.y, rhs.pos.y, epsilon) && feq(size.x, rhs.size.x, epsilon) && feq(size.y, rhs.size.y, epsilon);
    }

    /// True if point is inside rectangle (inclusive).
    [[nodiscard]] constexpr bool contains(vec2<T> point) const { return left() <= point.x && point.x <= right() && top() <= point.y && point.y <= bottom(); }

    /**
     * @brief Returns the normalized coordinates of a point inside this rectangle (aka local UV).
     * @param point The point to convert.
     * @return the normalized coordinates of the point relative to this rectangle.
     */
    [[nodiscard]] constexpr vec2<T> localCoords(vec2<T> point) const
        requires std::is_floating_point_v<T>
    {
        return {(point.x - pos.x) / size.x, (point.y - pos.y) / size.y};
    }

    /// Clamps a point to [0,1] normalized space of the rectangle.
    [[nodiscard]] constexpr vec2<T> clampPoint(vec2<T> point) const
        requires std::is_floating_point_v<T>
    {
        return glm::clamp(localCoords(point), T(0), T(1));
    }

    /// Clamps this rectangle to fit within another.
    [[nodiscard]] constexpr Rect<T> clampedTo(const Rect<T>& bounds) const
        requires std::is_floating_point_v<T>
    {
        Rect<T> ret;
        ret.pos = glm::clamp(pos, bounds.pos, bounds.pos + bounds.size);
        ret.size = glm::max(vec2<T>{0}, glm::min(bounds.pos + bounds.size - ret.pos, size - (ret.pos - pos)));
        return ret;
    }

    /// Returns transform that maps this rect to another.
    [[nodiscard]] constexpr Rect<T> relativeTo(const Rect<T>& to) const
        requires std::is_floating_point_v<T>
    {
        Rect<T> ret;
        ret.pos.x = (to.pos.x - pos.x) / size.x;
        ret.pos.y = (to.pos.y - pos.y) / size.y;
        ret.size.x = to.size.x / size.x;
        ret.size.y = to.size.y / size.y;
        return ret;
    }

    /// Returns a rectangle with the same position and size, but with floating-point precision.
    [[nodiscard]] constexpr Rect<f32> asF32() const { return {as<f32>(pos.x), as<f32>(pos.y), as<f32>(size.x), as<f32>(size.y)}; }
    /// Returns a rectangle with the same position and size, but with int precision
    [[nodiscard]] constexpr Rect<i32> asI32() const { return {as<i32>(pos.x), as<i32>(pos.y), as<i32>(size.x), as<i32>(size.y)}; }
};

/**
 * @ingroup MathTypes
 * @{
 */
using Rectu = Rect<u32>;  ///< 2D unsigned integer rectangle.
using Recti = Rect<i32>;  ///< 2D signed integer rectangle.
using Rectf = Rect<f32>;  ///< 2D floating-point rectangle.
using Rectd = Rect<f64>;  ///< 2D double-precision rectangle.
/// @}

/**
 * @brief Infinite line represented by a point and a normalized direction.
 * @ingroup MathTypes
 */
class Line2D {
  public:
    /// Constructs a default line at origin pointing right.
    Line2D() :
        origin_(0.0F),
        direction_(1.0F, 0.0F) {}

    /// Constructs a line from a point and direction (assumes direction is normalized).
    Line2D(vec2f origin, vec2f direction) :
        origin_(origin),
        direction_(direction) {}

    /// Constructs a line that passes through two distinct points.
    static Line2D fromPoints(vec2f a, vec2f b) { return {a, glm::normalize(b - a)}; }

    /// Gets a point on the line at parameter t (negative t is also valid).
    [[nodiscard]] constexpr vec2f at(f32 t) const { return origin_ + direction_ * t; }

    /// Projects a point onto the line, returning parameter t.
    [[nodiscard]] constexpr f32 project(vec2f point) const { return glm::dot(point - origin_, direction_); }

    /// Returns the closest point on the line to a given point.
    [[nodiscard]] constexpr vec2f closestPoint(vec2f point) const { return at(project(point)); }

    /// Returns the origin of the line.
    [[nodiscard]] constexpr vec2f origin() const { return origin_; }

    /// Returns the (normalized) direction of the line.
    [[nodiscard]] constexpr vec2f direction() const { return direction_; }

    /// Resturns true if the line contains a point (within epsilon).
    [[nodiscard]] bool contains(vec2f point, f32 epsilon = 1e-5F) const;

  private:
    vec2f origin_;
    vec2f direction_;
};

/**
 * @brief Line segment represented by two endpoints.
 * @ingroup MathTypes
 */
class LineSegment2D {
  public:
    /// Constructs a zero-length segment at origin.
    LineSegment2D() :
        a_(0.0F),
        b_(0.0F) {}

    /// Constructs a segment from two points.
    LineSegment2D(vec2f a, vec2f b) :
        a_(a),
        b_(b) {}

    /// Returns the start point of the segment.
    [[nodiscard]] constexpr vec2f getA() const { return a_; }

    /// Returns the end point of the segment.
    [[nodiscard]] constexpr vec2f getB() const { return b_; }

    /// Returns the direction vector (not normalized).
    [[nodiscard]] constexpr vec2f getDirection() const { return b_ - a_; }

    /// Returns the length of the segment.
    [[nodiscard]] f32 getLength() const { return glm::length(getDirection()); }

    /// Returns the midpoint of the segment.
    [[nodiscard]] constexpr vec2f getCenter() const { return (a_ + b_) * 0.5F; }

    /// Returns a point at parameter t ∈ [0,1].
    [[nodiscard]] constexpr vec2f at(f32 t) const { return a_ + getDirection() * t; }

    /// Returns the closest point on the segment to a given point.
    [[nodiscard]] vec2f closestPoint(vec2f point) const;

    /// True if the point lies on the segment (within epsilon).
    [[nodiscard]] bool contains(vec2f point, f32 epsilon = 1e-5F) const;

  private:
    vec2f a_;
    vec2f b_;
};

/**
 * @brief Ray represented by an origin and normalized direction.
 * @ingroup MathTypes
 */
class Ray2D {
  public:
    /// Constructs a default ray at origin pointing right.
    Ray2D() :
        origin_(0.0F),
        direction_(1.0F, 0.0F) {}

    /// Constructs a ray from a point and direction (assumes direction is normalized).
    Ray2D(vec2f origin, vec2f direction) :
        origin_(origin),
        direction_(direction) {}

    /// Constructs a ray from two points (from a to b).
    static Ray2D fromPoints(vec2f a, vec2f b) { return {a, glm::normalize(b - a)}; }

    /// Returns a point on the ray at parameter t ≥ 0.
    [[nodiscard]] constexpr vec2f at(f32 t) const { return origin_ + direction_ * t; }

    /// Projects a point onto the ray, returning clamped parameter t ≥ 0.
    [[nodiscard]] constexpr f32 project(vec2f point) const { return glm::max(0.0F, glm::dot(point - origin_, direction_)); }

    /// Returns the closest point on the ray to a given point.
    [[nodiscard]] constexpr vec2f getClosestPoint(vec2f point) const { return at(project(point)); }

    /// Returns the origin of the ray.
    [[nodiscard]] constexpr vec2f getOrigin() const { return origin_; }

    /// Returns the (normalized) direction of the ray.
    [[nodiscard]] constexpr vec2f getDirection() const { return direction_; }

  private:
    vec2f origin_;
    vec2f direction_;
};

/// Returns true if the point is inside the polygon (using winding number test).
bool isPointInsidePolygon(vec2f p, const std::vector<vec2f>& poly);

/// Returns true if polygon a is entirely inside polygon b.
bool isInside(const std::vector<vec2f>& a, const std::vector<vec2f>& b);

}  // namespace mle

namespace fmt {
template <typename T>
struct formatter<mle::Rect<T>> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::Rect<T>& rect, FormatContext& ctx) const {
        return format_to(ctx.out(), "[pos:{}, size:{}]", rect.pos, rect.size);
    }
};

template <>
struct formatter<mle::Line2D> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::Line2D& line, FormatContext& ctx) const {
        return format_to(ctx.out(), "[origin:{}, direction:{}]", line.origin(), line.direction());
    }
};

template <>
struct formatter<mle::LineSegment2D> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::LineSegment2D& segment, FormatContext& ctx) const {
        return format_to(ctx.out(), "[a:{}, b:{}]", segment.getA(), segment.getB());
    }
};

template <>

struct formatter<mle::Ray2D> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::Ray2D& ray, FormatContext& ctx) const {
        return format_to(ctx.out(), "[origin:{}, direction:{}]", ray.getOrigin(), ray.getDirection());
    }
};

}  // namespace fmt
