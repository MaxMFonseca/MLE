/**
 * @file Color.h
 * @brief Defines the mle::Color struct for color manipulation.
 */

#pragma once

#include <sol/forward.hpp>
#include <unordered_map>

#include "math/Types.h"
#include "mle/common/Logger.h"

namespace mle {
/**
 * Represents an RGBA color in **sRGB** color space with float components.
 *
 * Stored values are gamma-encoded and suitable for display. For lighting or blending,
 * use `toLinear()` to convert to linear space.
 */
struct Color : vec4f {
    Color(const Color&) = default;
    Color& operator=(const Color&) = default;
    Color(Color&&) = default;
    Color& operator=(Color&&) = default;

    /// Constructs a ZERO color.
    Color() = default;

    /// Constructs a color from an RGB vector and optional alpha.
    explicit Color(vec3f rgb, f32 alpha = 1.0F) :
        vec4f{rgb, alpha} {}

    /// Constructs a color from an 8-bit RGB vector and optional alpha.
    explicit Color(vec3u rgb, f32 alpha = 1.0F) :
        vec4f{vec3f{rgb} / MAX_U8_F, alpha} {}

    /// Constructs a color from an RGBA vector.
    explicit Color(vec4f rgba) :
        vec4f{rgba} {}

    /// Constructs a color from an 8-bit RGBA vector.
    explicit Color(vec4u rgba) :
        vec4f{vec4f(rgba) / MAX_U8_F} {}

    /// Constructs a color from individual float components.
    explicit Color(f32 r, f32 g, f32 b, f32 alpha = 1.0F) :
        vec4f{r, g, b, alpha} {}

    /// Constructs a color from individual 8-bit components.
    explicit Color(u32 r, u32 g, u32 b, u32 alpha = MAX_U8) noexcept :
        vec4f{vec4f{r, g, b, alpha} / MAX_U8_F} {}

    /// Constructs a color from a hexadecimal RGBA value (e.g., 0xRRGGBBAA).
    explicit Color(u32 hex) noexcept :
        Color{fromHex(hex)} {}

    ~Color() = default;

    /// Returns the color encoded as 0xRRGGBBAA.
    [[nodiscard]] u32 asRGBA() const;

    /// Returns the color encoded as 0xAARRGGBB.
    [[nodiscard]] u32 asARGB() const;

    /// Returns the color encoded as 0xAABBGGRR.
    [[nodiscard]] u32 asABGR() const;

    /// Converts the color to linear RGB space.
    [[nodiscard]] Color toLinear() const;

    /// Returns a lighter version of the color by scaling its value in HSV space.
    [[nodiscard]] Color lighten(f32 factor) const;

    /// Returns a copy of this color with the given alpha value.
    [[nodiscard]] Color withA(f32 new_a) const { return Color{r, g, b, new_a}; };

    /// Returns a copy of this color with the alpha scaled by the given factor.
    [[nodiscard]] Color withAScaled(f32 factor) const { return Color{r, g, b, a * factor}; };

    /// Parses a color from a string (e.g., hex string or named color).
    static Color fromString(const std::string& str);

    /// Parses a color from a Lua object (string, table, or integer).
    static Color fromLua(const sol::object& object);

    /// Converts a hexadecimal RGBA value to a color.
    static Color fromHex(u32 hex) noexcept;

    /// Converts a color to HSV.
    static vec3f toHSV(Color c);

    /// Converts the color to HSV.
    [[nodiscard]] vec3f toHSV() const { return toHSV(*this); };

    /// Creates a color from HSV values.
    static Color fromHSV(vec3f hsv);

    /// Adds the default engine-named colors to the global map.
    static void addEngineDefaultColors();

    /// Adds a color to the global map with the given name.
    static void addColor(const std::string& name, Color color);

    /// Adds a table of named colors from Lua.
    static void addColors(const sol::table& table);

    /// Retrieves a named color from the global map.
    static Color getColor(const std::string& name);

    /// Returns a random color.
    static Color random(u32 alpha = MAX_U8);

    /// Mixes two colors linearly based on the given factor.
    static Color mix(const Color& a, const Color& b, f32 factor);

    /// Registers the color type and utilities to the Lua environment.
    static void registerLuaTypes();

    static const Color ZERO;     ///< Fully transparent black.
    static const Color BLACK;    ///< Opaque black.
    static const Color WHITE;    ///< Opaque white.
    static const Color RED;      ///< Opaque red.
    static const Color GREEN;    ///< Opaque green.
    static const Color BLUE;     ///< Opaque blue.
    static const Color MAGENTA;  ///< Opaque magenta.
    static const Color YELLOW;   ///< Opaque yellow.
    static const Color CYAN;     ///< Opaque cyan.

    static constexpr u32 MAX_U8 = 255U;     ///< Maximum 8-bit unsigned value.
    static constexpr f32 MAX_U8_F = 255.F;  ///< Maximum 8-bit value as float.

    /// Scales the RGB channels by a scalar (alpha is preserved).
    Color operator*(f32 scalar) const { return Color{r * scalar, g * scalar, b * scalar, a}; };

    /// Adds two colors together (component-wise).
    Color operator+(const Color& other) const { return Color{r + other.r, g + other.g, b + other.b, a + other.a}; };

  private:
    /// Returns the global named color map.
    static std::unordered_map<std::string, Color>& colors();
};
}  // namespace mle

namespace fmt {
template <>
struct formatter<mle::Color> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::Color& color, FormatContext& ctx) const {
        return format_to(ctx.out(), "[[r:{}, g:{}, b:{}, a:{}] {:#x}]", color.r, color.g, color.b, color.a, color.asRGBA());
    }
};
}  // namespace fmt
