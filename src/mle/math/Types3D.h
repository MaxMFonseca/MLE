/**
 * @file Types3D.h
 * @brief 3D geometry primitives and utilities.
 */

#pragma once

#include <optional>

#include "Types.h"
#include "Types2D.h"
#include "mle/core/Assert.h"
#include "mle/utils/Types.h"

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

class Sphere;

/**
 * @brief Structure to represent near and far clipping planes.
 * @ingroup MathTypes
 */
struct NearFar {
    f32 near;  ///< Near clipping distance.
    f32 far;   ///< Far clipping distance.
};

class AABB {
  public:
    AABB() = default;  ///< Default constructor.

    /// Constructs an AABB from min and max points.
    AABB(vec3f min, vec3f max) :
        min_(min),
        max_(max) {}

    /// Returns the minimum corner of the box.
    [[nodiscard]] auto min() const { return min_; }

    /// Returns the maximum corner of the box.
    [[nodiscard]] auto max() const { return max_; }

    [[nodiscard]] auto width() const { return max_.x - min_.x; }  ///< Returns the width of the box.

    [[nodiscard]] auto height() const { return max_.y - min_.y; }  ///< Returns the height of the box.

    [[nodiscard]] auto depth() const { return max_.z - min_.z; }  ///< Returns the depth of the box.

    [[nodiscard]] auto size() const { return vec3f{width(), height(), depth()}; }  ///< Returns the extents of the box.

    [[nodiscard]] auto halfSize() const { return 0.5F * size(); }  ///< Returns the half extents of the box.

    /// Sets the minimum corner.
    void setMin(vec3f min) { min_ = min; }

    /// Sets the maximum corner.
    void setMax(vec3f max) { max_ = max; }

    /// Adds padding to the box by expanding both min and max corners.
    void addPaddin(vec3f padding) {
        min_ -= padding;
        max_ += padding;
    }

    /// Returns the size of the box (max - min).
    [[nodiscard]] vec3f extent() const { return max_ - min_; }

    /// Returns the center of the box.
    [[nodiscard]] vec3f center() const { return 0.5F * (min_ + max_); }

    /// Returns true if the box contains the given point.
    [[nodiscard]] bool contains(vec3f p) const {
        return (p.x >= min_.x && p.x <= max_.x) && (p.y >= min_.y && p.y <= max_.y) && (p.z >= min_.z && p.z <= max_.z);
    }

    /// Expands this box to include the given point.
    void expand(vec3f p) {
        min_ = glm::min(min_, p);
        max_ = glm::max(max_, p);
    }

    /// Expands this box to include another box.
    void expand(const AABB& other) {
        min_ = glm::min(min_, other.min_);
        max_ = glm::max(max_, other.max_);
    }

    /// Returns true if this box intersects another box.
    [[nodiscard]] bool intersects(const AABB& other) const {
        return (min_.x <= other.max_.x && max_.x >= other.min_.x) && (min_.y <= other.max_.y && max_.y >= other.min_.y) &&
               (min_.z <= other.max_.z && max_.z >= other.min_.z);
    }

    /// Returns all 8 corners of the box in no specific order.
    [[nodiscard]] std::array<vec3f, 8> corners() const;

    /**
     * @brief Checks whether this AABB intersects the given sphere.
     *
     * Uses closest-point distance test from the sphere center to the box.
     *
     * @param sphere Sphere to test.
     * @return True if the sphere intersects or touches the box.
     */
    [[nodiscard]] bool intersects(const Sphere& sphere) const;

    [[nodiscard]] Rectf translateToXZ(vec3f v) const;
    [[nodiscard]] Rectf translateToXZ(const mat4f& mat) const;  ///< Apply the translation part of the matrix to the AABB and return a 2D AABB.

  private:
    vec3f min_{0.0F};  ///< Minimum corner of the box.
    vec3f max_{0.0F};  ///< Maximum corner of the box.
};

/**
 * @brief A bounding sphere in 3D space.
 * @ingroup MathTypes
 */
class Sphere {
  public:
    Sphere() = default;  ///< Default constructor.

    /// Constructs a sphere from center and radius.
    Sphere(vec3f center, f32 radius) :
        center_(center),
        radius_(radius) {}

    /// Returns the center of the sphere.
    [[nodiscard]] auto center() const { return center_; }

    /// Returns the radius of the sphere.
    [[nodiscard]] auto radius() const { return radius_; }

    /// Sets the center.
    void setCenter(vec3f center) { center_ = center; }

    /// Sets the radius.
    void setRadius(f32 radius) { radius_ = radius; }

    /// Checks whether a point lies inside or on the surface of the sphere.
    [[nodiscard]] bool contains(vec3f point) const { return glm::distance2(point, center_) <= radius_ * radius_; }

    /// Returns the signed distance from the surface (negative = inside).
    [[nodiscard]] f32 signedDistance(vec3f point) const { return glm::distance(point, center_) - radius_; }

  private:
    vec3f center_{0.0F};  ///< Center of the sphere.
    f32 radius_ = 1.0F;   ///< Radius of the sphere.
};

// /**
//  * @brief Identifies which face of a box a point lies on, within a given epsilon tolerance.
//  *
//  * This function assumes the point is already on the surface of the given box (within numerical error),
//  * and returns the face it belongs to. It compares the point’s coordinates against the six box faces
//  * using `feq` with the provided `epsilon` value.
//  *
//  * This function is intended for use in contexts where the point is guaranteed to lie on the box surface.
//  * Failure to match any face is considered unreachable and should never occur in correct usage.
//  *
//  * @param box The box whose face is being queried.
//  * @param point A point on the surface of the box.
//  * @param epsilon Tolerance for floating-point comparisons.
//  * @return The specific face of the box the point lies on.
//  */
// BoxFace pointFace(const Boxf& box, vec3f point, f32 epsilon);

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
     * @brief Computes the intersection interval between this ray and an AABB.
     *
     * Returns the distances along the ray where it enters and exits the box.
     * If there's no intersection, returns std::nullopt.
     *
     * @param box Axis-aligned bounding box to test against.
     * @return Optional NearFar with entry and exit distances.
     */
    [[nodiscard]] std::optional<NearFar> intersect(const AABB& box) const;

    /**
     * @brief Tests whether this ray intersects the given sphere.
     *
     * @param sphere Sphere to test against.
     * @return True if the ray intersects the sphere.
     */
    [[nodiscard]] bool intersects(const Sphere& sphere) const;

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
    Plane(vec3f origin, vec3f normal) :
        origin_(origin),
        normal_(glm::normalize(normal)) {}

    /// Returns the point on the plane.
    [[nodiscard]] auto origin() const { return origin_; }

    /// Returns the plane normal (normalized).
    [[nodiscard]] auto normal() const { return normal_; }

    /// Sets the point on the plane.
    void setOrigin(vec3f point) { origin_ = point; }

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
    vec3f origin_{};  ///< A point on the plane.
    vec3f normal_{};  ///< Normal vector of the plane.
};

/**
 * @brief A finite rectangular plane in 3D space.
 * @ingroup MathTypes
 */
class RectPlane {
  public:
    RectPlane() = default;  ///< Default constructor.

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

    static RectPlane fromSquareLBCorner(vec3f corner, vec3f normal, f32 size);

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

/// A simple 3D polygon represented a list of 3D vertices.
class Polygon3D {
  public:
    /// Constructs an empty polygon.
    Polygon3D() = default;

    /// Constructs a polygon from a list of 3D points.
    explicit Polygon3D(std::vector<vec3f> verts);

    void addVert(vec3f v) { verts_.emplace_back(v); }  ///< Adds a vertex to the polygon.

    /// Returns the vertices.
    [[nodiscard]] const std::vector<vec3f>& vertices() const { return verts_; }

    /// Returns the number of vertices.
    [[nodiscard]] usize vertCount() const { return verts_.size(); }

    /// Returns the centroid of the polygon.
    [[nodiscard]] vec3f center() const;

    /// Returns the normal of the polygon using Newell.
    [[nodiscard]] vec3f normal() const;

    /// Sorts the vertices in counter-clockwise order.
    void sortCCW();

    /// Returns a random vector that is perpendicular.
    [[nodiscard]] static vec3f anyPerpendicular(vec3f v);

    /// Returns the polygon on plane y [x, 0, z]
    std::vector<vec2f> xz();

  private:
    std::vector<vec3f> verts_;  ///< List of vertices
};

/// A view frustum defined by six planes in world space.
class Frustum {
  public:
    Frustum() = default;

    /// Constructs a frustum by extracting planes from a combined projection-view matrix.
    explicit Frustum(const mat4f& m);

    /// Computes the signed distance from the point to the nearest plane.
    [[nodiscard]] f32 signedDist(const vec3f& p) const;

    /// Returns true if the point is inside the frustum.
    [[nodiscard]] bool contains(const vec3f& p) const;

    /// Returns true if all four corners of the RectPlane are inside or intersect the frustum.
    [[nodiscard]] bool contains(const RectPlane& plane) const;
    /// Returns true if the sphere is partially or fully inside the frustum.
    [[nodiscard]] bool contains(const Sphere& s) const;

    /**
     * @brief Tests whether the AABB is partially or fully inside the frustum.
     *
     * Uses conservative plane-vs-corner test.
     *
     * @param box The axis-aligned bounding box to test.
     * @return True if box is at least partially inside the frustum.
     */
    [[nodiscard]] bool contains(const AABB& box) const;

    /// Returns a polygon that represents the intersection of the frustum with the Y=0 plane.
    [[nodiscard]] Polygon3D intersectWithY0() const;

    [[nodiscard]] std::array<vec3f, 8> getCorners() const;

    [[nodiscard]] static vec3f calcCenter(const std::array<vec3f, 8>& corners);

  private:
    struct ABCDPlane {
        f32 a, b, c, d;

        /// Normalizes the plane coefficients.
        void normalize();

        /// Computes the signed distance from a point to this plane.
        [[nodiscard]] f32 signedDist(const vec3f& p) const { return (a * p.x) + (b * p.y) + (c * p.z) + d; }

        /// Computes the intersection point of 3 planes using Cramer's rule.
        static vec3f intersect(const Frustum::ABCDPlane& p1, const Frustum::ABCDPlane& p2, const Frustum::ABCDPlane& p3);
    };

  private:
    ABCDPlane l_, r_, t_, b_, n_, f_;  ///< Left, right, top, bottom, near, far planes
};
/// A plane in 3D space represented by the equation ax + by + cz + d = 0.
}  // namespace mle

namespace fmt {
template <>
struct formatter<mle::AABB> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::AABB& box, FormatContext& ctx) const {
        return format_to(ctx.out(), "[min:{}, max:{}]", box.min(), box.max());
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
        return format_to(ctx.out(), "[origin:{}, normal:{}]", p.origin(), p.normal());
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
        auto corners = rp.getEdges();
        return format_to(ctx.out(), "[center:{}, normal:{}, width:{}, height:{}, corners: [{}, {}, {}, {}]]", rp.center(), rp.normal(), rp.width(), rp.height(),
                         corners.at(0), corners.at(1), corners.at(2), corners.at(3));
    }
};
}  // namespace fmt
