/**
 * @file Types3D.h
 * @brief 3D geometry primitives and utilities.
 */

#pragma once

#include <optional>

#include "Types.h"

namespace mle {
/**
 * @ingroup MathTypes
 * @{
 */
template <typename T>
using Point3 = vec3<T>;
using Point3u = Point3<u32>;
using Point3i = Point3<i32>;
using Point3f = Point3<f32>;
using Point3d = Point3<f64>;

template <typename T>
using Extent3 = vec3<T>;
using Extent3u = Extent3<u32>;
using Extent3i = Extent3<i32>;
using Extent3f = Extent3<f32>;
using Extent3d = Extent3<f64>;
/// @}

/**
 * @brief Structure to represent near and far clipping planes.
 * @ingroup MathTypes
 */
struct NearFar {
    f32 near;  ///< Near clipping distance.
    f32 far;   ///< Far clipping distance.
};

/**
 * @brief A 3D axis-aligned bounding box (AABB).
 * @ingroup MathTypes
 *
 * @tparam T Scalar type (e.g., f32, i32)
 */
template <typename T>
struct Box {
    vec3<T> pos;      ///< Minimum corner of the box (lower-left-front).
    Extent3<T> size;  ///< Extent of the box along each axis.

    Box() = default;

    /// Constructs a box from raw components.
    Box(T x, T y, T z, T width, T height, T depth) :
        pos(x, y, z),
        size(width, height, depth) {}

    /// Constructs a box from a position and size vector.
    Box(vec3<T> pos, Extent3<T> size) :
        pos(pos),
        size(size) {}

    [[nodiscard]] T left() const { return pos.x; }            ///< Left edge (x position).
    [[nodiscard]] T right() const { return pos.x + size.x; }  ///< Right edge (x + width)
    [[nodiscard]] T bottom() const { return pos.y; }          ///< Bottom edge (y position)
    [[nodiscard]] T top() const { return pos.y + size.y; }    ///< Top edge (y + height)
    [[nodiscard]] T front() const { return pos.z; }           ///< Front edge (z position)
    [[nodiscard]] T back() const { return pos.z + size.z; }   ///< Back edge (z + depth)

    [[nodiscard]] T width() const { return size.x; }   ///< size.x
    [[nodiscard]] T height() const { return size.y; }  ///< size.y
    [[nodiscard]] T depth() const { return size.z; }   ///< size.z

    [[nodiscard]] vec3<T> min() const { return pos; }         ///< Minimum corner of the box.
    [[nodiscard]] vec3<T> max() const { return pos + size; }  ///< Maximum corner of the box.

    [[nodiscard]] vec3<T> center() const { return pos + (size / 2); }  ///< Center of the box.

    /// True if width, height, and depth are greater than 0.
    [[nodiscard]] constexpr bool valid() const { return width() > 0 && height() > 0 && depth() > 0; }

    /// Returns true if the point is inside or on the boundary of the box.
    [[nodiscard]] bool contains(vec3<T> point) const {
        return point.x >= left() && point.x <= right() && point.y >= bottom() && point.y <= top() && point.z >= front() && point.z <= back();
    }

    /// Returns true if this box completely contains another box.
    [[nodiscard]] bool contains(const Box<T>& other) const {
        return left() <= other.left() && right() >= other.right() && bottom() <= other.bottom() && top() >= other.top() && front() <= other.front() &&
               back() >= other.back();
    }

    /// Moves the box by the given offset.
    void move(vec3<T> offset) { pos += offset; }

    /// Compares two boxes for exact equality.
    bool operator==(const Box<T>& other) const { return pos == other.pos && size == other.size; }
};

/**
 * @ingroup MathTypes
 * @{
 */
using Boxu = Box<u32>;  ///< 3D unsigned integer box.
using Boxi = Box<i32>;  ///< 3D signed integer box.
using Boxf = Box<f32>;  ///< 3D floating-point box.
using Boxd = Box<f64>;  ///< 3D double-precision box.
/// @}

/**
 * @brief Identifies which face of a box a point lies on, within a given epsilon tolerance.
 *
 * This function assumes the point is already on the surface of the given box (within numerical error),
 * and returns the face it belongs to. It compares the point’s coordinates against the six box faces
 * using `feq` with the provided `epsilon` value.
 *
 * This function is intended for use in contexts where the point is guaranteed to lie on the box surface.
 * Failure to match any face is considered unreachable and should never occur in correct usage.
 *
 * @param box The box whose face is being queried.
 * @param point A point on the surface of the box.
 * @param epsilon Tolerance for floating-point comparisons.
 * @return The specific face of the box the point lies on.
 */
BoxFace pointFace(const Boxf& box, vec3f point, f32 epsilon);

/**
 * @brief A 3D ray with origin and normalized direction.
 * @ingroup MathTypes
 *
 * Provides support for intersection testing and directional operations.
 */
class Ray3f {
  public:
    Ray3f() = default;  ///< Default constructor.

    /// Constructs a ray from origin and direction.
    Ray3f(vec3f origin, vec3f direction) :
        origin_(origin),
        direction_(glm::normalize(direction)) {
        calcInvDir();
    }

    /// Returns the ray origin.
    [[nodiscard]] auto origin() const { return origin_; }

    /// Returns the ray direction (normalized).
    [[nodiscard]] auto direction() const { return direction_; }

    /// Sets the ray origin.
    void setOrigin(vec3f origin) { origin_ = origin; }

    /// Sets the ray direction (assumed normalized).
    void setDirectionPreNormalized(vec3f direction) {
        direction_ = direction;
        calcInvDir();
    }

    /// Sets the ray direction and normalizes it.
    void setDirection(vec3f direction) { setDirectionPreNormalized(glm::normalize(direction)); }

    /// Sets the ray direction to target - origin.
    void setTarget(vec3f target) { setDirection(target - origin_); }

    /// Returns the inverse direction (for fast AABB tests).
    [[nodiscard]] vec3f inverseDirection() const { return inv_direction_; }

    /// Computes a point at distance t along the ray.
    [[nodiscard]] vec3f pointAt(f32 t) const { return origin_ + direction_ * t; }

    /**
     * @brief Computes the intersection interval between this ray and a box.
     *
     * Returns the distances along the ray where it enters and exits the box.
     * If there's no intersection, returns std::nullopt.
     *
     * @param box Box to test intersection against.
     * @return Optional NearFar with entry and exit distances.
     */
    [[nodiscard]] std::optional<NearFar> intersect(const Boxf& box) const;

  private:
    /// Recomputes the inverse direction vector.
    void calcInvDir() {
        inv_direction_.x = direction_.x == 0 ? inf<f32>() : 1.0F / direction_.x;
        inv_direction_.y = direction_.y == 0 ? inf<f32>() : 1.0F / direction_.y;
        inv_direction_.z = direction_.z == 0 ? inf<f32>() : 1.0F / direction_.z;
    }

  private:
    vec3f origin_{};         ///< Origin of the ray.
    vec3f direction_{};      ///< Normalized direction.
    vec3f inv_direction_{};  ///< Inverse direction for slab tests.
};

/**
 * @brief A finite 3D line segment defined by two endpoints.
 * @ingroup MathTypes
 */
class LineSegment3f {
  public:
    LineSegment3f() = default;  ///< Default constructor.

    /// Construct from start and end points.
    LineSegment3f(vec3f start, vec3f end) :
        start_(start),
        end_(end) {}

    /// Construct from a ray and a length.
    LineSegment3f(const Ray3f& ray, f32 len) :
        start_(ray.origin()),
        end_(ray.origin() + ray.direction() * len) {}

    /// Start point of the segment.
    [[nodiscard]] auto start() const { return start_; }

    /// End point of the segment.
    [[nodiscard]] auto end() const { return end_; }

    /// Set start point.
    void setStart(vec3f start) { start_ = start; }

    /// Set end point.
    void setEnd(vec3f end) { end_ = end; }

    /// Compare for equality.
    bool operator==(const LineSegment3f& other) const { return start_ == other.start_ && end_ == other.end_; }

  private:
    vec3f start_{};  ///< First endpoint.
    vec3f end_{};    ///< Second endpoint.
};

/**
 * @brief A plane in 3D space defined by a point and a normal.
 * @ingroup MathTypes
 */
class Plane {
  public:
    Plane() = default;  ///< Default constructor.

    /// Construct from point and normal.
    Plane(vec3f point, vec3f normal) :
        point_(point),
        normal_(glm::normalize(normal)) {}

    /// Returns the point on the plane.
    [[nodiscard]] auto point() const { return point_; }

    /// Returns the plane normal (normalized).
    [[nodiscard]] auto normal() const { return normal_; }

    /// Sets the point on the plane.
    void setPoint(vec3f point) { point_ = point; }

    /// Sets and normalizes the normal vector.
    void setNormal(vec3f normal) { normal_ = glm::normalize(normal); }

    /// Sets the normal directly, assuming it's already normalized.
    void setNormalPreNormalized(vec3f normal) { normal_ = normal; }

    /**
     * @brief Computes the intersection of this plane with a ray.
     *
     * @param ray Ray to test.
     * @return Distance from ray origin to intersection point, or nullopt if parallel.
     */
    [[nodiscard]] std::optional<f32> intersect(const Ray3f& ray) const;

  private:
    vec3f point_{};   ///< A point on the plane.
    vec3f normal_{};  ///< Normal vector of the plane.
};

/**
 * @brief A finite rectangular plane in 3D space.
 * @ingroup MathTypes
 */
class RectPlane {
  public:
    RectPlane() = default;  ///< Default constructor.

    /// Construct a rectangular plane from center, normal, width, and height.
    RectPlane(vec3f center, vec3f normal, f32 width, f32 height) :
        center_(center),
        normal_(glm::normalize(normal)),
        width_(width),
        height_(height) {
        recomputeBasis();
    }

    /// Returns the center of the plane.
    [[nodiscard]] auto center() const { return center_; }

    /// Returns the normal vector (normalized).
    [[nodiscard]] auto normal() const { return normal_; }

    /// Returns the tangent (local X axis).
    [[nodiscard]] auto tangent() const { return tangent_; }

    /// Returns the bitangent (local Y axis).
    [[nodiscard]] auto bitangent() const { return bitangent_; }

    /// Returns the width of the rectangle.
    [[nodiscard]] auto width() const { return width_; }

    /// Returns the height of the rectangle.
    [[nodiscard]] auto height() const { return height_; }

    /// Sets the center of the rectangle.
    void setCenter(vec3f center) { center_ = center; }

    /// Sets and normalizes the normal, then recomputes tangent/bitangent.
    void setNormal(vec3f normal) {
        normal_ = glm::normalize(normal);
        recomputeBasis();
    }

    /// Sets the width of the rectangle.
    void setWidth(f32 width) { width_ = width; }

    /// Sets the height of the rectangle.
    void setHeight(f32 height) { height_ = height; }

    /// Sets both width and height.
    void setSize(f32 width, f32 height) {
        width_ = width;
        height_ = height;
    }

    [[nodiscard]] std::array<vec3f, 4> getEdges() const;

    /**
     * @brief Computes intersection with a ray, if inside bounds.
     *
     * @param ray Ray to test.
     * @return Distance from ray origin to intersection point, or nullopt if no intersection or outside bounds.
     */
    [[nodiscard]] std::optional<f32> intersect(const Ray3f& ray) const;

  private:
    /// Recomputes tangent and bitangent from the current normal.
    void recomputeBasis();

    vec3f center_{};     ///< Center of the rectangular plane.
    vec3f normal_{};     ///< Normal vector (unit length).
    vec3f tangent_{};    ///< Local X axis (width direction).
    vec3f bitangent_{};  ///< Local Y axis (height direction).
    f32 width_{};        ///< Width along tangent.
    f32 height_{};       ///< Height along bitangent.
};
}  // namespace mle

namespace fmt {
template <typename T>
struct formatter<mle::Box<T>> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::Box<T>& b, FormatContext& ctx) const {
        return format_to(ctx.out(), "[pos:{}, size:{}]", b.pos, b.size);
    }
};

template <>
struct formatter<mle::Ray3f> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::Ray3f& r, FormatContext& ctx) const {
        return format_to(ctx.out(), "[origin:{}, direction:{}]", r.origin(), r.direction());
    }
};

template <>
struct formatter<mle::LineSegment3f> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::LineSegment3f& l, FormatContext& ctx) const {
        return format_to(ctx.out(), "[start:{}, end:{}]", l.start(), l.end());
    }
};

template <>
struct formatter<mle::Plane> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::Plane& p, FormatContext& ctx) const {
        return format_to(ctx.out(), "[point:{}, normal:{}]", p.point(), p.normal());
    }
};

template <>
struct formatter<mle::NearFar> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::NearFar& nf, FormatContext& ctx) const {
        return format_to(ctx.out(), "[near:{}, far:{}]", nf.near, nf.far);
    }
};

template <>
struct formatter<mle::RectPlane> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::RectPlane& rp, FormatContext& ctx) const {
        return format_to(ctx.out(), "[center:{}, normal:{}, width:{}, height:{}]", rp.center(), rp.normal(), rp.width(), rp.height());
    }
};
}  // namespace fmt
