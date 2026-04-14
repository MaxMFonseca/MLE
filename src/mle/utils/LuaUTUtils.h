#pragma once

#include "mle/lua/Lua.h"
#include "mle/utils/Color.h"
#include "mle/utils/Stopwatch.h"

namespace mle::lua {
inline void makeUTColor(Lua& lua) {
    auto ut = lua.newUsertype<Color>("Color",
                                     sol::constructors<Color(), Color(vec3f, f32), Color(vec4f), Color(vec4u), Color(f32, f32, f32, f32),
                                                       Color(u32, u32, u32, u32), Color(u32), Color(std::string), Color(sol::object)>(),
                                     sol::base_classes, sol::bases<vec4f>());
    ut["r"] = &Color::r;
    ut["g"] = &Color::g;
    ut["b"] = &Color::b;
    ut["a"] = &Color::a;
    ut["mix"] = &Color::mix;
    ut["lighten"] = &Color::lighten;
    ut["withA"] = &Color::withA;
    ut["toLinear"] = &Color::toLinear;
    ut["toSRGB"] = &Color::toSRGB;
    ut["random"] = &Color::random;
    ut["fromHSV"] = [](f32 h, f32 s, f32 v) { return Color::fromHSVA({h, s, v, 1.0F}); };
    ut["toHSV"] = &Color::toHSV;
}

inline void makeUTStopwatch(Lua& lua) {
    auto ut = lua.newUsertype<Stopwatch>("Stopwatch", sol::constructors<Stopwatch()>());
    ut["reset"] = &Stopwatch::reset;
    ut["elapsedSecInt"] = &Stopwatch::elapsedSecInt;
    ut["elapsedSecFloat"] = &Stopwatch::elapsedSecFloat;
    ut["elapsedMSFloat"] = &Stopwatch::elapsedMSFloat;
}

}  // namespace mle::lua
