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
    [[nodiscard]] constexpr bool contains(vec2<T> point) const
        requires std::is_floating_point_v<T>
    {
        return left() <= point.x && point.x <= right() && top() <= point.y && point.y <= bottom();
    }

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

}  // namespace mle

namespace fmt {
template <typename T>
struct formatter<mle::Rect<T>> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::Rect<T>& rect, FormatContext& ctx) const {
        return format_to(ctx.out(), "[pos:{}, size:{}]", rect.pos, rect.size);
    }
};
}  // namespace fmt
