#pragma once

#include "mle/lua/Lua.h"
#include "mle/math/Types.h"
#include "mle/math/Types2D.h"

namespace mle::lua {
inline void makeUTMathTypes(Lua& lua) {
    lua.newUsertype<vec2i>("Vec2i", sol::constructors<vec2i(i32, i32)>(), "x", &vec2i::x, "y", &vec2i::y);
    lua.newUsertype<vec3i>("Vec3i", sol::constructors<vec3i(i32, i32, i32)>(), "x", &vec3i::x, "y", &vec3i::y, "z", &vec3i::z);
    lua.newUsertype<vec4i>("Vec4i", sol::constructors<vec4i(i32, i32, i32, i32)>(), "x", &vec4i::x, "y", &vec4i::y, "z", &vec4i::z, "w", &vec4i::w);

    lua.newUsertype<vec2f>("Vec2f", sol::constructors<vec2f(f32, f32)>(), "x", &vec2f::x, "y", &vec2f::y);
    lua.newUsertype<vec3f>("Vec3f", sol::constructors<vec3f(f32, f32, f32)>(), "x", &vec3f::x, "y", &vec3f::y, "z", &vec3f::z);
    lua.newUsertype<vec4f>("Vec4f", sol::constructors<vec4f(f32, f32, f32, f32)>(), "x", &vec4f::x, "y", &vec4f::y, "z", &vec4f::z, "w", &vec4f::w);

    lua.newUsertype<Rectf>("Rectf", sol::constructors<Rectf(f32, f32, f32, f32), Rectf(vec2f, vec2f)>(), "pos", &Rectf::pos, "size", &Rectf::size);
    lua.newUsertype<Recti>("Recti", sol::constructors<Recti(i32, i32, i32, i32), Recti(vec2i, vec2i)>(), "pos", &Recti::pos, "size", &Recti::size);
}
}  // namespace mle::lua
