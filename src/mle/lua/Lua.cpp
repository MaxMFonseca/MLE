#include "Lua.h"

#include "Types.h"
#include "mle/common/Assert.h"
#include "mle/common/Color.h"
#include "mle/common/Logger.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types2D.h"
#include "mle/core/Core.h"

namespace mle {
namespace {
inline void atLuaPanic(sol::optional<std::string> maybe_msg) {
    MLE_C("Lua panic occurred!");
    if (maybe_msg && !maybe_msg.value().empty()) {
        MLE_C("Lua error: {}", maybe_msg.value());
    } else if (!maybe_msg) {
        MLE_C("Lua provided no error message.");
    }
    core::unrecoverable("Lua panic occurred! aborting application.");
}
}  // namespace

void Lua::init() {
    MLE_I("Creating Lua instance");

    sol_.set_panic(sol::c_call<decltype(&atLuaPanic), &atLuaPanic>);

    sol_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::jit, sol::lib::math, sol::lib::string, sol::lib::table);

    if (!sol_["jit"].valid()) {
        core::unrecoverable("Lua JIT is not available!");
    }

    auto package_path = sol_["package"]["path"];
    package_path = package_path.get<std::string>() + ";res/lua/?.lua";

    MLE_D("{}", sol_["_VERSION"].get<std::string>());
    MLE_D("{}", sol_["jit"]["version"].get<std::string>());
    MLE_D("{}", sol_["package"]["path"].get<std::string>());

    setGlobal("Utils", require("mle/utils"));

    registerCommonTypes();
}

void Lua::registerCommonTypes() {
    newUsertype<vec2i>("vec2i", sol::constructors<vec2i(i32, i32)>(), "x", &vec2i::x, "y", &vec2i::y);
    newUsertype<vec3i>("vec3i", sol::constructors<vec3i(i32, i32, i32)>(), "x", &vec3i::x, "y", &vec3i::y, "z", &vec3i::z);
    newUsertype<vec4i>("vec4i", sol::constructors<vec4i(i32, i32, i32, i32)>(), "x", &vec4i::x, "y", &vec4i::y, "z", &vec4i::z, "w", &vec4i::w);

    newUsertype<vec2f>("vec2f", sol::constructors<vec2f(f32, f32)>(), "x", &vec2f::x, "y", &vec2f::y);
    newUsertype<vec3f>("vec3f", sol::constructors<vec3f(f32, f32, f32)>(), "x", &vec3f::x, "y", &vec3f::y, "z", &vec3f::z);
    newUsertype<vec4f>("vec4f", sol::constructors<vec4f(f32, f32, f32, f32)>(), "x", &vec4f::x, "y", &vec4f::y, "z", &vec4f::z, "w", &vec4f::w);

    newUsertype<Rectf>("rectf", sol::constructors<Rectf(f32, f32, f32, f32), Rectf(vec2f, vec2f)>(), "pos", &Rectf::pos, "size", &Rectf::size);

    registerCommonTypesColor();
}

void Lua::registerCommonTypesColor() {
    auto ut = newUsertype<Color>(
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

sol::object Lua::require(const std::string& module_name) {
    MLE_D("Requiring Lua module: {}", module_name);

    MLE_ASSERT_LOG(!module_name.empty(), "Module name must not be empty");
    MLE_ASSERT_LOG(!module_name.ends_with(".lua"), "Module name must not end with .lua");

    return sol_.script_file("res/lua/" + module_name + ".lua");
}

sol::table Lua::getTable(const std::string& name) {
    return sol_.get<sol::table>(name);
}

sol::table Lua::createTable() {
    return sol_.create_table();
}

sol::table Lua::createTable(const std::string& name) {
    MLE_D("Creating global table: {}", name);
    return sol_.create_named_table(name);
}

sol::table Lua::createTable(const sol::table& table, bool deep) {
    sol::table copy = createTable();

    if (!deep) {
        for (auto& [key, value] : table) {
            copy[key] = value;
        }
        return copy;
    }

    std::vector<std::pair<sol::table, sol::table>> stack;
    stack.emplace_back(table, copy);

    while (!stack.empty()) {
        auto [src, dst] = stack.back();
        stack.pop_back();

        for (auto& [key, value] : src) {
            if (value.get_type() == sol::type::table) {
                sol::table new_table = createTable();
                dst[key] = new_table;
                stack.emplace_back(value.as<sol::table>(), new_table);
            } else {
                dst[key] = value;
            }
        }
    }

    return copy;
}

void Lua::mergeTables(sol::table& dst, const sol::table& src) {  // NOLINT
    for (const auto& [key, val] : src) {
        if (val.get_type() == sol::type::table) {
            auto dst_val = dst[key];
            if (!dst_val.valid() || !dst_val.is<sol::table>()) {
                dst[key] = createTable(val, true);
            } else {
                auto dst_val_table = dst_val.get<sol::table>();
                mergeTables(dst_val_table, val.as<sol::table>());
            }
        } else {
            dst[key] = val;
        }
    }
}

}  // namespace mle
