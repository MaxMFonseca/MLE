#pragma once

#include "mle/lua/Lua.h"
#include "mle/utils/Color.h"
#include "mle/utils/Stopwatch.h"

namespace mle::lua {
inline void makeUTColor(Lua& lua) {
    auto ut = lua.newUsertype<Color>(
        "Color", sol::constructors<Color(), Color(vec3f, f32), Color(vec4f), Color(vec4u), Color(f32, f32, f32, f32), Color(u32, u32, u32, u32), Color(u32)>(),
        sol::base_classes, sol::bases<vec4f>());
    ut["r"] = &Color::r;
    ut["g"] = &Color::g;
    ut["b"] = &Color::b;
    ut["a"] = &Color::a;
    ut["fromString"] = &Color::fromString;
    ut["fromLua"] = &Color::fromLua;
    ut["addColor"] = &Color::addColor;
    ut["getColor"] = &Color::getColor;
    ut["mix"] = &Color::mix;
    ut["lighten"] = &Color::lighten;
    ut["withA"] = &Color::withA;
}

inline void makeUTStopwatch(Lua& lua) {
    auto ut = lua.newUsertype<Stopwatch>("Stopwatch", sol::constructors<Stopwatch()>());
    ut["reset"] = &Stopwatch::reset;
    ut["elapsedSecInt"] = &Stopwatch::elapsedSecInt;
    ut["elapsedSecFloat"] = &Stopwatch::elapsedSecFloat;
    ut["elapsedMSFloat"] = &Stopwatch::elapsedMSFloat;
}

}  // namespace mle::lua
