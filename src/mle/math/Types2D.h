/**
 * @file Types2D.h
 * @brief 2D geometry primitives and utilities.
 */
#pragma once

#include <span>

#include "Intersect2D.h"
#include "Types.h"
#include "mle/utils/Utils.h"

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
class Rect {
  public:
    /// Constructs a ZERO rectangle.
    constexpr Rect() = default;

    /// Constructs a rectangle from raw components.
    constexpr Rect(T x, T y, T width, T height) {
        setPos(x, y);
        setSize(width, height);
    }

    /// Constructs a rectangle from a position and extent
    constexpr Rect(vec2<T> pos, Extent2<T> size) :
        Rect(pos.x, pos.y, size.x, size.y) {}

    constexpr void setPosX(T v) { pos_.x = v; }          ///< Sets the x position of the rectangle.
    constexpr void setPosY(T v) { pos_.y = v; }          ///< Sets the y position of the rectangle.
    constexpr void setPos(vec2<T> pos) { pos_ = pos; }   ///< Sets the position of the rectangle.
    constexpr void setPos(T x, T y) { setPos({x, y}); }  ///< Sets the position of the rectangle.

    /// Sets the width of the rectangle.
    constexpr void setSizeX(T v) {
        size_.x = v;
        normalizeX();
    }
    constexpr void setWidth(T v) { setSizeX(v); }  ///< Sets the width of the rectangle.
    constexpr void setW(T v) { setSizeX(v); }      ///< Sets the width of the rectangle.

    /// Sets the height of the rectangle.
    constexpr void setSizeY(T v) {
        size_.y = v;
        normalizeY();
    }
    constexpr void setHeight(T v) { setSizeY(v); }  ///< Sets the height of the rectangle.
    constexpr void setH(T v) { setSizeY(v); }       ///< Sets the height of the rectangle.

    /// Sets the size of the rectangle.
    constexpr void setSize(vec2<T> size) {
        size_ = size;
        normalizeX();
        normalizeY();
    }
    constexpr void setSize(T width, T height) { setSize({width, height}); }  ///< Sets the size of the rectangle.
    constexpr void setWH(T width, T height) { setSize({width, height}); }    ///< Sets the size of the rectangle.

    /// Sets the left edge of the rectangle, adjusting position and width.
    constexpr void setLeft(T v) {
        T delta = v - left();
        pos_.x += delta;
        size_.x -= delta;
        normalizeX();
    }
    constexpr void setL(T v) { setLeft(v); }  ///< Sets the left edge of the rectangle.

    /// Sets the right edge of the rectangle, adjusting width.
    constexpr void setRight(T v) {
        size_.x = v - pos_.x;
        normalizeX();
    }
    constexpr void setR(T v) { setRight(v); }  ///< Sets the right edge of the rectangle.

    /// Sets the top edge of the rectangle, adjusting position and height.
    constexpr void setTop(T v) {
        T delta = v - top();
        pos_.y += delta;
        size_.y -= delta;
        normalizeY();
    }
    constexpr void setT(T v) { setTop(v); }  ///< Sets the top edge of the rectangle.

    /// Sets the bottom edge of the rectangle, adjusting height.
    constexpr void setBottom(T v) {
        size_.y = v - pos_.y;
        normalizeY();
    }
    constexpr void setB(T v) { setBottom(v); }  ///< Sets the bottom edge of the rectangle.

    /// Sets position and size.
    constexpr void set(T x, T y, T width, T height) {
        setPos(x, y);
        setSize(width, height);
    }
    /// Sets position and size.
    constexpr void set(vec2<T> pos, vec2<T> size) {
        setPos(pos);
        setSize(size);
    }

    /// Expands this rectangle to include a point.
    constexpr void expand(vec2<T> p) {
        const vec2<T> old_min = min();
        const vec2<T> old_max = max();

        const vec2<T> new_min{glm::min(old_min.x, p.x), glm::min(old_min.y, p.y)};
        const vec2<T> new_max{glm::max(old_max.x, p.x), glm::max(old_max.y, p.y)};

        pos_ = new_min;
        size_ = new_max - new_min;
    }

    /// Expands this rectangle to include a set of points.
    constexpr void expand(std::span<const vec2<T>> points) {
        if (points.empty()) {
            return;
        }

        vec2<T> min_v = pos_;
        vec2<T> max_v = pos_ + size_;

        for (const auto& p : points) {
            min_v.x = glm::min(min_v.x, p.x);
            min_v.y = glm::min(min_v.y, p.y);
            max_v.x = glm::max(max_v.x, p.x);
            max_v.y = glm::max(max_v.y, p.y);
        }

        pos_ = min_v;
        size_ = max_v - min_v;
    }

    /// Creates a rectangle that encloses a set of points.
    [[nodiscard]] static constexpr Rect<T> fromPoints(std::span<const vec2<T>> points) {
        assert(!points.empty());
        Rect<T> r{points[0].x, points[0].y, 0, 0};
        r.expand(points);
        return r;
    }

    /// Expands this rectangle to include another rectangle.
    constexpr void expand(const Rect<T>& other) {
        expand(other.min());
        expand(other.max());
    }

    /// Creates a rectangle representing the intersection of this and another rectangle.
    [[nodiscard]] constexpr Rect<T> intersection(const Rect<T>& other) const {
        const vec2<T> a_min = pos_;
        const vec2<T> a_max = pos_ + size_;
        const vec2<T> b_min = other.pos();
        const vec2<T> b_max = other.pos() + other.size();

        const vec2<T> i_min{glm::max(a_min.x, b_min.x), glm::max(a_min.y, b_min.y)};
        const vec2<T> i_max{glm::min(a_max.x, b_max.x), glm::min(a_max.y, b_max.y)};

        const vec2<T> i_size{glm::max(T(0), i_max.x - i_min.x), glm::max(T(0), i_max.y - i_min.y)};

        return Rect<T>(i_min, i_size);
    }

    /// Translates rectangle by a given offset.
    constexpr void move(vec2<T> offset) { pos_ += offset; }

    [[nodiscard]] constexpr vec2<T> pos() const { return pos_; }    ///< Top-left corner of the rectangle.
    [[nodiscard]] constexpr vec2<T> size() const { return size_; }  ///< Size (width, height) of the rectangle.

    [[nodiscard]] constexpr T left() const { return pos_.x; }              ///< Left edge (x position).
    [[nodiscard]] constexpr T right() const { return pos_.x + size_.x; }   ///< Right edge (x + width).
    [[nodiscard]] constexpr T top() const { return pos_.y; }               ///< Top edge (y position).
    [[nodiscard]] constexpr T bottom() const { return pos_.y + size_.y; }  ///< Bottom edge (y + height).

    [[nodiscard]] constexpr T width() const { return size_.x; }   ///< Width of the rectangle.
    [[nodiscard]] constexpr T height() const { return size_.y; }  ///< Height of the rectangle.

    [[nodiscard]] constexpr vec2<T> min() const { return pos_; }          ///< Minimum corner of the rectangle.
    [[nodiscard]] constexpr vec2<T> max() const { return pos_ + size_; }  ///< Maximum corner of the rectangle.

    /// Center of the rectangle.
    [[nodiscard]] constexpr vec2<T> center() const
        requires std::is_floating_point_v<T>
    {
        return pos_ + (size_ * 0.5F);
    }

    /// True if the rectangles are equal. Only enabled for integer types
    [[nodiscard]] constexpr bool operator==(const Rect<T>& other) const
        requires(!std::is_floating_point_v<T>)
    {
        return pos_ == other.pos_ && size_ == other.size_;
    }

    /// True if the rectangles are equal within a given epsilon.
    [[nodiscard]] constexpr bool almostEqual(const Rect<T>& rhs, T epsilon) const
        requires std::is_floating_point_v<T>
    {
        return feq(pos_.x, rhs.pos_.x, epsilon) && feq(pos_.y, rhs.pos_.y, epsilon) && feq(size_.x, rhs.size_.x, epsilon) && feq(size_.y, rhs.size_.y, epsilon);
    }

    /**
     * @brief Returns the normalized coordinates of a point inside this rectangle (aka local UV).
     * @param point The point to convert.
     * @return the normalized coordinates of the point relative to this rectangle.
     */
    [[nodiscard]] constexpr vec2<T> localCoords(vec2<T> point) const
        requires std::is_floating_point_v<T>
    {
        return {(point.x - pos_.x) / size_.x, (point.y - pos_.y) / size_.y};
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
        ret.pos_ = glm::clamp(pos_, bounds.pos_, bounds.pos_ + bounds.size_);
        ret.size_ = glm::max(vec2<T>{0}, glm::min(bounds.pos_ + bounds.size_ - ret.pos_, size_ - (ret.pos_ - pos_)));
        return ret;
    }

    /// Returns transform that maps this rect to another.
    [[nodiscard]] constexpr Rect<T> relativeTo(const Rect<T>& to) const
        requires std::is_floating_point_v<T>
    {
        Rect<T> ret;
        ret.pos_.x = (to.pos_.x - pos_.x) / size_.x;
        ret.pos_.y = (to.pos_.y - pos_.y) / size_.y;
        ret.size_.x = to.size_.x / size_.x;
        ret.size_.y = to.size_.y / size_.y;
        return ret;
    }

    /// Returns a rectangle with the same position and size, but with floating-point precision.
    [[nodiscard]] constexpr Rect<f32> asF32() const { return {as<f32>(pos_.x), as<f32>(pos_.y), as<f32>(size_.x), as<f32>(size_.y)}; }
    /// Returns a rectangle with the same position and size, but with int precision
    [[nodiscard]] constexpr Rect<i32> asI32() const { return {as<i32>(pos_.x), as<i32>(pos_.y), as<i32>(size_.x), as<i32>(size_.y)}; }
    /// Returns {top, bottom, left, right}.
    [[nodiscard]] constexpr auto asTBLR() const { return std::array<T, 4>{top(), bottom(), left(), right()}; }

    template <typename U>
    [[nodiscard]] constexpr bool intersect(const U& o, f32 eps = FloatTolerance<f32>::REL) const
        requires std::is_floating_point_v<T>
    {
        return Intersect2D::intersect(*this, o, eps);
    }

  private:
    /// Normalizes the rectangle to ensure positive width.
    constexpr void normalizeX() {
        if (size_.x < 0) {
            pos_.x += size_.x;
            size_.x = -size_.x;
        }
    }
    /// Normalizes the rectangle to ensure positive height.
    constexpr void normalizeY() {
        if (size_.y < 0) {
            pos_.y += size_.y;
            size_.y = -size_.y;
        }
    }

  private:
    vec2<T> pos_;   ///< Top-left corner of the rectangle.
    vec2<T> size_;  ///< The size of the rectangle.
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

Rectf makeAABB2D(std::span<const vec2f> points);  ///< Creates an AABB that encloses a set of points.

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

    template <typename U>
    [[nodiscard]] constexpr bool intersect(const U& o, f32 eps = FloatTolerance<f32>::REL) const {
        return Intersect2D::intersect(*this, o, eps);
    }

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

    template <typename U>
    [[nodiscard]] constexpr bool intersect(const U& o, f32 eps = FloatTolerance<f32>::REL) const {
        return Intersect2D::intersect(*this, o, eps);
    }

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

    template <typename U>
    [[nodiscard]] constexpr bool intersect(const U& o, f32 eps = FloatTolerance<f32>::REL) const {
        return Intersect2D::intersect(*this, o, eps);
    }

  private:
    vec2f origin_;
    vec2f direction_;
};

/**
 * @brief A simple 2D polygon represented by an ordered list of 2D vertices.
 * @ingroup MathTypes
 */
class Polygon2f {
  public:
    Polygon2f() = default;  ///< Default constructor.

    /// Constructs a polygon from a list of 2D points.
    explicit Polygon2f(std::vector<vec2f> verts) :
        verts_(std::move(verts)) {}

    /// Returns the vertices.
    [[nodiscard]] const std::vector<vec2f>& vertices() const { return verts_; }

    /// Returns the i-th vertex.
    [[nodiscard]] vec2f vertex(usize i) const { return verts_.at(i); }

    [[nodiscard]] auto boundingBox() const { return Rectf::fromPoints(verts_); };

    /// Returns the number of vertices.
    [[nodiscard]] u64 vertexCount() const { return verts_.size(); }

    /// Adds a vertex to the polygon.
    void addVertex(vec2f v) { verts_.emplace_back(v); }

    /// Returns the centroid of the polygon.
    [[nodiscard]] vec2f center() const;

    /// Returns the signed area of the polygon.
    [[nodiscard]] f32 area() const;

    /// Sorts the vertices counter-clockwise.
    void sortCCW();

    template <typename U>
    [[nodiscard]] constexpr bool intersect(const U& o, f32 eps = FloatTolerance<f32>::REL) const {
        return Intersect2D::intersect(*this, o, eps);
    }

  private:
    std::vector<vec2f> verts_;  ///< List of vertices.
};

/**
 * @brief A 2D circle represented by a center and a radius.
 * @ingroup MathTypes
 */
class Circle {
  public:
    Circle() = default;  ///< Default constructor.

    /// Constructs a circle from center and radius.
    Circle(vec2f center, f32 radius) :
        center_(center),
        radius_(radius),
        radius2_(radius * radius) {}

    /// Returns the center of the circle.
    [[nodiscard]] auto center() const { return center_; }

    /// Returns the radius of the circle.
    [[nodiscard]] auto radius() const { return radius_; }

    /// Returns the squared radius of the circle.
    [[nodiscard]] auto radius2() const { return radius2_; }

    /// Sets the center of the circle.
    void setCenter(vec2f center) { center_ = center; }

    /// Sets the radius of the circle.
    void setRadius(f32 radius) {
        radius_ = radius;
        radius2_ = radius * radius;
    }

    /// Returns the signed distance from a point to the circle boundary.
    [[nodiscard]] f32 signedDistance(vec2f point) const { return glm::length(point - center_) - radius_; }

    /// Returns the closest point on the circle boundary to the given point.
    [[nodiscard]] vec2f closestPoint(vec2f point) const;

    template <typename U>
    [[nodiscard]] constexpr bool intersect(const U& o, f32 eps = FloatTolerance<f32>::REL) const {
        return Intersect2D::intersect(*this, o, eps);
    }

  private:
    vec2f center_{};  ///< Center of the circle.
    f32 radius_{};    ///< Radius of the circle.
    f32 radius2_{};   ///< Cached squared radius.
};
}  // namespace mle

namespace fmt {
template <typename T>
struct formatter<mle::Rect<T>> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::Rect<T>& rect, FormatContext& ctx) const {
        return format_to(ctx.out(), "[pos:{}, size:{}]", rect.pos(), rect.size());
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

template <>
struct formatter<mle::Polygon2f> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::Polygon2f& polygon, FormatContext& ctx) const {
        std::string verts_str;
        for (const auto& v : polygon.vertices()) {
            verts_str += fmt::format("{} ", v);
        }
        if (!verts_str.empty()) {
            verts_str.pop_back();
            return format_to(ctx.out(), "[vertices:{{{}}}]", verts_str);
        }
        return format_to(ctx.out(), "[vertices:[]]");
    }
};
}  // namespace fmt
