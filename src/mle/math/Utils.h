#pragma once

#include "mle/math/Types.h"

namespace mle {
/// Returns a rough approximation of the square root of a floating-point number.
f32 roughSqrt(f32 x);

/// Rounds up to the next power of two (returns same value if already a power of two).
template <std::unsigned_integral T>
constexpr T roundUpToPow2(T v) {
    if (v == 0) {
        return 1;
    }
    return T{1} << (std::bit_width(v - 1));
}

/// Value for floating-point comparisons.
template <typename T>
struct FloatTolerance {
    static constexpr T REL = T(8) * std::numeric_limits<T>::epsilon();
    static constexpr T ABS = T(1e-6);
};

/// Safety margin for floating-point comparisons.
template <typename T>
[[nodiscard]] constexpr T floatTolerance(T a, T b, T rel = FloatTolerance<T>::REL, T abs = FloatTolerance<T>::ABS)
    requires std::is_floating_point_v<T>
{
    const T ma = std::abs(a);
    const T mb = std::abs(b);
    const T scaled = rel * (ma > mb ? ma : mb);
    return scaled > abs ? scaled : abs;
}
/// Alias for floatTolerance
template <typename T>
[[nodiscard]] constexpr T ftol(T a, T b, T rel = FloatTolerance<T>::REL, T abs = FloatTolerance<T>::ABS) {
    return floatTolerance(a, b, rel, abs);
}

/// Floating-point equality check within a tolerance.
template <typename T>
[[nodiscard]] constexpr bool almostEqual(T a, T b, T rel = FloatTolerance<T>::REL, T abs = FloatTolerance<T>::ABS)
    requires std::is_floating_point_v<T>
{
    return std::abs(a - b) <= floatTolerance(a, b, rel, abs);
}
/// Alias for almostEqual
template <typename T>
[[nodiscard]] constexpr bool feq(T a, T b, T rel = FloatTolerance<T>::REL, T abs = FloatTolerance<T>::ABS) {
    return almostEqual(a, b, rel, abs);
}

/// FLoating-point less than comparison within a tolerance.
template <typename T>
[[nodiscard]] constexpr bool lessThanEps(T a, T b, T rel = FloatTolerance<T>::REL, T abs = FloatTolerance<T>::ABS)
    requires std::is_floating_point_v<T>
{
    return a < b - floatTolerance(a, b, rel, abs);
}
/// Alias for lessThanEps
template <typename T>
[[nodiscard]] constexpr bool flt(T a, T b, T rel = FloatTolerance<T>::REL, T abs = FloatTolerance<T>::ABS) {
    return lessThanEps(a, b, rel, abs);
}

/// FLoating-point greater than comparison within a tolerance.
template <typename T>
[[nodiscard]] constexpr bool greaterThanEps(T a, T b, T rel = FloatTolerance<T>::REL, T abs = FloatTolerance<T>::ABS)
    requires std::is_floating_point_v<T>
{
    return a > b + floatTolerance(a, b, rel, abs);
}
/// Alias for greaterThanEps
template <typename T>
[[nodiscard]] constexpr bool fgt(T a, T b, T rel = FloatTolerance<T>::REL, T abs = FloatTolerance<T>::ABS) {
    return greaterThanEps(a, b, rel, abs);
}

/// Floating-point greater than comparison within a tolerance.
template <typename T>
[[nodiscard]] constexpr bool lessThanEqualEps(T a, T b, T rel = FloatTolerance<T>::REL, T abs = FloatTolerance<T>::ABS)
    requires std::is_floating_point_v<T>
{
    return !greaterThanEps(a, b, rel, abs);
}
/// Alias for lessThanEqualEps
template <typename T>
[[nodiscard]] constexpr bool fle(T a, T b, T rel = FloatTolerance<T>::REL, T abs = FloatTolerance<T>::ABS) {
    return lessThanEqualEps(a, b, rel, abs);
}

/// Floating-point greater than or equal comparison within a tolerance.
template <typename T>
[[nodiscard]] constexpr bool greaterThanEqualEps(T a, T b, T rel = FloatTolerance<T>::REL, T abs = FloatTolerance<T>::ABS)
    requires std::is_floating_point_v<T>
{
    return !lessThanEps(a, b, rel, abs);
}
/// Alias for greaterThanEqualEps
template <typename T>
[[nodiscard]] constexpr bool fge(T a, T b, T rel = FloatTolerance<T>::REL, T abs = FloatTolerance<T>::ABS) {
    return greaterThanEqualEps(a, b, rel, abs);
}

/// Floating-point three-way comparison within a tolerance.
template <typename T>
[[nodiscard]] constexpr int floatCompare(T a, T b, T rel = FloatTolerance<T>::REL, T abs = FloatTolerance<T>::ABS)
    requires std::is_floating_point_v<T>
{
    if (almostEqual(a, b, rel, abs)) {
        return 0;
    }
    return (a < b) ? -1 : 1;
}
/// Alias for floatCompare
template <typename T>
[[nodiscard]] constexpr int fcmp(T a, T b, T rel = FloatTolerance<T>::REL, T abs = FloatTolerance<T>::ABS) {
    return floatCompare(a, b, rel, abs);
}

}  // namespace mle
