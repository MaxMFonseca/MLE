#include "Color.h"

#include <random>

#include "mle/core/Logger.h"
#include "mle/lua/Lua.h"
#include "mle/lua/Utils.h"

namespace mle {
const Color Color::ZERO = Color{0U};
const Color Color::ONE = Color{MAX_U8, MAX_U8, MAX_U8, MAX_U8};
const Color Color::BLACK = Color{0U, 0U, 0U};
const Color Color::WHITE = Color{MAX_U8, MAX_U8, MAX_U8};
const Color Color::RED = Color{MAX_U8, 0U, 0U};
const Color Color::GREEN = Color{0U, MAX_U8, 0U};
const Color Color::BLUE = Color{0U, 0U, MAX_U8};
const Color Color::MAGENTA = Color{MAX_U8, 0U, MAX_U8};
const Color Color::YELLOW = Color{MAX_U8, MAX_U8, 0U};
const Color Color::CYAN = Color{0U, MAX_U8, MAX_U8};
const Color Color::GRAY = Color(0.5F, 0.5F, 0.5F);
const Color Color::NQB = Color(0.05F, 0.05F, 0.05F);
const Color Color::NQW = Color(0.85F, 0.85F, 0.85F);

Color Color::fromString(const std::string& str) {
    if (str.size() > 2) {
        if (str[0] == '0' && str[1] == 'x') {
            return Color{static_cast<u32>(std::stoul(str, nullptr, 16))};
        }
        if (str[0] == '#') {
            auto s2 = str.substr(1);
            if (s2.size() == 6) {
                s2 += "FF";
            }
            return Color{static_cast<u32>(std::stoul(s2, nullptr, 16))};
        }
    }

    return getColor(str);
}

Color Color::fromLua(const sol::object& object) {
    if (!object.valid()) {
        MLE_W("Invalid color object, returning WHITE");
        return WHITE;
    }

    switch (object.get_type()) {
        case sol::type::number:
            return Color{object.as<u32>()};
        case sol::type::string:
            return fromString(object.as<std::string>());
        case sol::type::table: {
            auto table = object.as<sol::table>();

            if (const auto t1_r = table[1]; lua::valid<std::string>(t1_r)) {
                Color ret;
                ret = fromString(table[1].get<std::string>());
                if (const auto t2_r = table[2]; lua::valid<f32>(t2_r)) {
                    ret.a = t2_r.get<f32>();
                }
                return ret;
            }

            f32 r = NAN, g = NAN, b = NAN;
            if (!lua::tryGetList<f32>(table, r, g, b)) {
                MLE_W("Invalid Lua color table, returning WHITE");
                return WHITE;
            }

            if (r > 1 || g > 1 || b > 1) {
                u32 a = lua::getIdxOr(table, 4, MAX_U8);
                return Color{as<u32>(r), as<u32>(g), as<u32>(b), a};
            }

            f32 a = lua::getIdxOr(table, 4, 1.F);
            return Color{r, g, b, a};
        }
        case sol::type::userdata: {
            if (object.is<Color>()) {
                return object.as<Color>();
            }
            if (object.is<vec4u>()) {
                return Color{object.as<vec4u>()};
            }
            if (object.is<vec4f>()) {
                return Color{object.as<vec4f>()};
            }
            if (object.is<vec3u>()) {
                return Color{object.as<vec3u>()};
            }
            if (object.is<vec3f>()) {
                return Color{object.as<vec3f>()};
            }
        }
        default:
            break;
    }
    MLE_UNREACHABLE_LOG("Invalid color type");
}

Color Color::fromHex(u32 hex) noexcept {
    Color ret;
    ret.r = static_cast<f32>((hex >> 24U) & 0xFFU) / MAX_U8_F;
    ret.g = static_cast<f32>((hex >> 16U) & 0xFFU) / MAX_U8_F;
    ret.b = static_cast<f32>((hex >> 8U) & 0xFFU) / MAX_U8_F;
    ret.a = static_cast<f32>(hex & 0xFFU) / MAX_U8_F;
    return ret;
}

std::unordered_map<std::string, Color>& Color::colors() {
    static std::unordered_map<std::string, Color> colors;

    return colors;
}

void Color::addEngineDefaultColors() {
    std::vector<std::pair<std::string, Color>> default_colors = {{"ZERO", ZERO},   {"ONE", ONE},   {"BLACK", BLACK},     {"WHITE", WHITE},   {"RED", RED},
                                                                 {"GREEN", GREEN}, {"BLUE", BLUE}, {"MAGENTA", MAGENTA}, {"YELLOW", YELLOW}, {"CYAN", CYAN},
                                                                 {"GRAY", GRAY},   {"NQB", NQB},   {"NQW", NQW}};

    MLE_D("Adding engine default colors");
    for (const auto& [name, color] : default_colors) {
        addColor(name, color);
    }
}

void Color::addColor(const std::string& name, Color color) {
    if (colors().contains(name)) {
        MLE_W("Color '{}' already exists! overwriting...", name);
    }
    MLE_D("Adding color '{}': srgb:{} linear:{}", name, color, color.toLinear());
    colors()[name] = color;
}

void Color::addColors(const sol::table& table) {
    for (const auto& [key, value] : table) {
        MLE_ASSERT(key.is<std::string>());
        addColor(key.as<std::string>(), fromLua(value));
    }
}

Color Color::getColor(const std::string& name) {
    auto it = colors().find(name);
    if (it == colors().end()) {
        MLE_W("Color '{}' not found! returning white", name);
        return {};
    }
    return it->second;
}

Color Color::random(u32 alpha) {
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_int_distribution<u32> dist(0, MAX_U8);
    return Color{dist(rng), dist(rng), dist(rng), alpha};
}

u32 Color::asRGBA() const {
    return static_cast<u32>(r * MAX_U8) << 24U | static_cast<u32>(g * MAX_U8) << 16U | static_cast<u32>(b * MAX_U8) << 8U | static_cast<u32>(a * MAX_U8);
};

u32 Color::asARGB() const {
    return static_cast<u32>(a * MAX_U8) << 24U | static_cast<u32>(r * MAX_U8) << 16U | static_cast<u32>(g * MAX_U8) << 8U | static_cast<u32>(b * MAX_U8);
}

u32 Color::asABGR() const {
    return static_cast<u32>(a * MAX_U8) << 24U | static_cast<u32>(b * MAX_U8) << 16U | static_cast<u32>(g * MAX_U8) << 8U | static_cast<u32>(r * MAX_U8);
}

Color Color::toLinear() const {
    Color ret;

    if (r <= 0.04045F) {
        ret.r = r / 12.92F;
    } else {
        ret.r = glm::pow((r + 0.055F) / 1.055F, 2.4F);
    }

    if (g <= 0.04045F) {
        ret.g = g / 12.92F;
    } else {
        ret.g = glm::pow((g + 0.055F) / 1.055F, 2.4F);
    }

    if (b <= 0.04045F) {
        ret.b = b / 12.92F;
    } else {
        ret.b = glm::pow((b + 0.055F) / 1.055F, 2.4F);
    }

    ret.a = a;
    return ret;
}

Color Color::mix(const Color& a, const Color& b, f32 factor) {
    MLE_ASSERT_LOG(factor >= 0.0F && factor <= 1.0F, "Factor must be in the range [0, 1]. {}", factor);
    auto ret = a * (1.0F - factor) + b * factor;
    ret.a = (a.a * (1.0F - factor)) + (b.a * factor);
    return ret;
}

Color Color::fromHSV(vec3f hsv) {
    f32 h = hsv.x, s = hsv.y, v = hsv.z;

    f32 c = v * s;
    f32 x = c * (1 - std::fabs(std::fmod(h / 60.0F, 2.0F) - 1));
    f32 m = v - c;

    f32 r = 0, g = 0, b = 0;

    if (h >= 0 && h < 60) {
        r = c, g = x, b = 0;
    } else if (h >= 60 && h < 120) {
        r = x, g = c, b = 0;
    } else if (h >= 120 && h < 180) {
        r = 0, g = c, b = x;
    } else if (h >= 180 && h < 240) {
        r = 0, g = x, b = c;
    } else if (h >= 240 && h < 300) {
        r = x, g = 0, b = c;
    } else if (h >= 300 && h < 360) {
        r = c, g = 0, b = x;
    }

    r += m;
    g += m;
    b += m;

    return Color{r, g, b, 1.0F};
}

vec3f Color::toHSV(Color c) {
    auto r = c.r, g = c.g, b = c.b;
    f32 max_component = std::max({r, g, b});
    float min_component = std::min({r, g, b});
    float delta = max_component - min_component;

    float h = 0.0F;
    if (delta > 0.0F) {
        if (max_component == r) {
            h = 60.0F * (std::fmod(((g - b) / delta), 6.0F));
        } else if (max_component == g) {
            h = 60.0F * (((b - r) / delta) + 2.0F);
        } else if (max_component == b) {
            h = 60.0F * (((r - g) / delta) + 4.0F);
        }
    }

    if (h < 0.0F) {
        h += 360.0F;
    }

    float s = (max_component == 0.0F) ? 0.0F : (delta / max_component);
    float v = max_component;
    return {h, s, v};
}

Color Color::lighten(f32 factor) const {
    auto hsv = toHSV();
    hsv.z *= factor;
    return fromHSV(hsv);
}

std::vector<Color> Color::lerpCount(Color a, Color b, usize count) {
    std::vector<Color> ret;
    if (count == 0) {
        return ret;
    }
    if (count == 1) {
        return {mix(a, b, 0.5F)};
    }

    ret.reserve(count);
    for (usize i = 0; i < count; ++i) {
        f32 t = as<f32>(i) / as<f32>(count - 1);
        ret.emplace_back(mix(a, b, t));
    }

    return ret;
}
}  // namespace mle
