#pragma once

#include "mle/lua/Lua.h"
#include "mle/math/Types.h"
#include "mle/math/Types2D.h"

namespace mle::lua {
inline void makeUTMathTypes(Lua& lua) {
    lua.newUsertype<vec2i>("vec2i", sol::constructors<vec2i(i32, i32)>(), "x", &vec2i::x, "y", &vec2i::y);
    lua.newUsertype<vec3i>("vec3i", sol::constructors<vec3i(i32, i32, i32)>(), "x", &vec3i::x, "y", &vec3i::y, "z", &vec3i::z);
    lua.newUsertype<vec4i>("vec4i", sol::constructors<vec4i(i32, i32, i32, i32)>(), "x", &vec4i::x, "y", &vec4i::y, "z", &vec4i::z, "w", &vec4i::w);

    lua.newUsertype<vec2f>("vec2f", sol::constructors<vec2f(f32, f32)>(), "x", &vec2f::x, "y", &vec2f::y);
    lua.newUsertype<vec3f>("vec3f", sol::constructors<vec3f(f32, f32, f32)>(), "x", &vec3f::x, "y", &vec3f::y, "z", &vec3f::z);
    lua.newUsertype<vec4f>("vec4f", sol::constructors<vec4f(f32, f32, f32, f32)>(), "x", &vec4f::x, "y", &vec4f::y, "z", &vec4f::z, "w", &vec4f::w);

    lua.newUsertype<Rectf>("rectf", sol::constructors<Rectf(f32, f32, f32, f32), Rectf(vec2f, vec2f)>(), "pos", &Rectf::pos, "size", &Rectf::size);
}
}  // namespace mle::lua
