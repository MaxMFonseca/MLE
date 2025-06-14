/**
 * @file Types.h
 * @brief Core type aliases and math utilities.
 *
 * Defines consistent scalar and vector types used throughout the engine,
 * along with helper functions for arithmetic limits, comparisons, and conversions.
 * Also integrates GLM types and enforces engine-wide precision and formatting policies.
 */

#pragma once

#include <spdlog/fmt/fmt.h>

#include <cmath>
#include <cstdint>
#include <limits>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/quaternion.hpp>

namespace mle {

using namespace std::chrono_literals;  // NOLINT

/**
 * @defgroup MathTypes Math Types
 * @brief All math types used throughout the engine.
 *
 * This module provides convenient and consistent type aliases for numeric,
 * vector, quaternion, and matrix types using GLM and standard fixed-size types.
 * @{
 */
using f32 = float_t;   ///< 32-bit floating point
using f64 = double_t;  ///< 64-bit floating point

using u8 = uint8_t;    ///< 8-bit unsigned integer
using u16 = uint16_t;  ///< 16-bit unsigned integer
using u32 = uint32_t;  ///< 32-bit unsigned integer
using u64 = uint64_t;  ///< 64-bit unsigned integer

using i8 = int8_t;    ///< 8-bit signed integer
using i16 = int16_t;  ///< 16-bit signed integer
using i32 = int32_t;  ///< 32-bit signed integer
using i64 = int64_t;  ///< 64-bit signed integer

template <typename T>
using vec2 = glm::vec<2, T>;  ///< A 2D vector

using vec2f = vec2<f32>;    ///< 2D vector of f32
using vec2d = vec2<f64>;    ///< 2D vector of f64
using vec2f64 = vec2d;      ///< Alias for vec2<f64>
using vec2u = vec2<u32>;    ///< 2D vector of u32
using vec2u64 = vec2<u64>;  ///< 2D vector of u64
using vec2i = vec2<i32>;    ///< 2D vector of i32
using vec2i64 = vec2<i64>;  ///< 2D vector of i64

template <typename T>
using vec3 = glm::vec<3, T>;  ///< A 3D vector

using vec3f = vec3<f32>;    ///< 3D vector of f32
using vec3d = vec3<f64>;    ///< 3D vector of f64
using vec3f64 = vec3d;      ///< Alias for vec3<f64>
using vec3u = vec3<u32>;    ///< 3D vector of u32
using vec3u64 = vec3<u64>;  ///< 3D vector of u64
using vec3i = vec3<i32>;    ///< 3D vector of i32
using vec3i64 = vec3<i64>;  ///< 3D vector of i64

template <typename T>
using vec4 = glm::vec<4, T>;  ///< A 4D vector

using vec4f = vec4<f32>;    ///< 4D vector of f32
using vec4d = vec4<f64>;    ///< 4D vector of f64
using vec4f64 = vec4d;      ///< Alias for vec4<f64>
using vec4u = vec4<u32>;    ///< 4D vector of u32
using vec4u64 = vec4<u64>;  ///< 4D vector of u64
using vec4i = vec4<i32>;    ///< 4D vector of i32
using vec4i64 = vec4<i64>;  ///< 4D vector of i64

using quat = glm::quat;  ///< Quaternion rotation

template <typename T>
using mat2 = glm::mat<2, 2, T>;  ///< 2x2 matrix

using mat2f = mat2<f32>;  ///< 2x2 matrix of f32

template <typename T>
using mat4 = glm::mat<4, 4, T>;  ///< 4x4 matrix

using mat4f = mat4<f32>;  ///< 4x4 matrix of f32
/// @}

/// Returns the maximum representable value of an arithmetic type.
template <typename T>
[[nodiscard]] constexpr T max()
    requires std::is_arithmetic_v<T>
{
    return std::numeric_limits<T>::max();
}

/// Returns the minimum representable value of an arithmetic type.
template <typename T>
[[nodiscard]] constexpr T min()
    requires std::is_arithmetic_v<T>
{
    return std::numeric_limits<T>::min();
}

/// Returns the quiet NaN value of a floating-point type.
template <typename T>
[[nodiscard]] constexpr T nan()
    requires std::is_floating_point_v<T>
{
    return std::numeric_limits<T>::quiet_NaN();
}

/// Returns the positive infinity value of a floating-point type.
template <typename T>
[[nodiscard]] constexpr T inf()
    requires std::is_floating_point_v<T>
{
    return std::numeric_limits<T>::infinity();
}

/// Compares two floating-point numbers for approximate equality.
///
/// Epsilon must be provided explicitly to reflect the comparison context.
template <typename T>
[[nodiscard]] constexpr bool feq(T a, T b, T epsilon)
    requires std::is_floating_point_v<T>
{
    return std::abs(a - b) < epsilon;
}

/// Computes 2 raised to the power of x (i.e., the x-th bit set).
template <typename T>
[[nodiscard]] constexpr T bit(T x)
    requires std::is_integral_v<T>
{
    return static_cast<T>(1) << x;
}

/// Returns the decimal (fractional) part of a floating-point number.
template <typename T>
[[nodiscard]] constexpr T decimalPart(T n)
    requires std::is_floating_point_v<T>
{
    return n - std::floor(n);
}

/// Returns the sign of a number: 1 if positive, -1 if negative, 0 if zero.
template <typename T>
[[nodiscard]] constexpr int sign(T value)
    requires std::is_arithmetic_v<T>
{
    return (T(0) < value) - (value < T(0));
}

/// Enumeration for axis indices.
enum class Axis : u8 {
    X = 0,       ///< X-axis
    Y = 1,       ///< Y-axis
    Z = 2,       ///< Z-axis
    W = 3,       ///< W-axis
    WIDTH = 0,   ///< X-axis
    HEIGHT = 1,  ///< Y-axis
    DEPTH = 2    ///< Z-axis
};

/**
 * @brief Enum representing the faces of a box in 3D space.
 *
 * The faces are named according to their orientation in the standard
 * right-handed coordinate system.
 */
enum class BoxFace : u8 {
    TOP = 0,     ///< Top face (+Y)
    BOTTOM = 1,  ///< Bottom face (-Y)
    LEFT = 2,    ///< Left face (-X)
    RIGHT = 3,   ///< Right face (+X)
    FRONT = 4,   ///< Front face (+Z)
    BACK = 5,    ///< Back face (-Z)

    T = TOP,
    B = BOTTOM,
    L = LEFT,
    R = RIGHT,

    POSITIVE_Y = TOP,
    NEGATIVE_Y = BOTTOM,
    NEGATIVE_X = LEFT,
    POSITIVE_X = RIGHT,
    POSITIVE_Z = FRONT,
    NEGATIVE_Z = BACK
};

}  // namespace mle

namespace fmt {
template <class T>
struct formatter<mle::vec2<T>> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::vec2<T>& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[{}, {}]", v.x, v.y);
    }
};

template <class T>
struct formatter<mle::vec3<T>> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::vec3<T>& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[{}, {}, {}]", v.x, v.y, v.z);
    }
};

template <class T>
struct formatter<mle::vec4<T>> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::vec4<T>& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[{}, {}, {}, {}]", v.x, v.y, v.z, v.w);
    }
};

template <>
struct formatter<mle::quat> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::quat& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[{}, {}, {}, {}]", v.x, v.y, v.z, v.w);
    }
};

template <class T>
struct formatter<mle::mat2<T>> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::mat2<T>& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[{}, {}]", v[0], v[1]);
    }
};

template <class T>
struct formatter<mle::mat4<T>> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::mat4<T>& v, FormatContext& ctx) const {
        return format_to(ctx.out(), "[{}, {}, {}, {}]", v[0], v[1], v[2], v[3]);
    }
};

template <>
struct formatter<mle::Axis> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::Axis v, FormatContext& ctx) const {
        switch (v) {
            case mle::Axis::X:
                return format_to(ctx.out(), "X");
            case mle::Axis::Y:
                return format_to(ctx.out(), "Y");
            case mle::Axis::Z:
                return format_to(ctx.out(), "Z");
            case mle::Axis::W:
                return format_to(ctx.out(), "W");
        }
    }
};

template <>
struct formatter<mle::BoxFace> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::BoxFace v, FormatContext& ctx) const {
        switch (v) {
            case mle::BoxFace::TOP:
                return format_to(ctx.out(), "TOP");
            case mle::BoxFace::BOTTOM:
                return format_to(ctx.out(), "BOTTOM");
            case mle::BoxFace::LEFT:
                return format_to(ctx.out(), "LEFT");
            case mle::BoxFace::RIGHT:
                return format_to(ctx.out(), "RIGHT");
            case mle::BoxFace::FRONT:
                return format_to(ctx.out(), "FRONT");
            case mle::BoxFace::BACK:
                return format_to(ctx.out(), "BACK");
        }
    }
};
}  // namespace fmt
