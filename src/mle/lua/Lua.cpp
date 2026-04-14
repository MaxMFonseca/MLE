#include "Lua.h"

#include "mle/core/Assert.h"
#include "mle/core/Core.h"
#include "mle/core/Logger.h"
#include "mle/math/LuaUTMathTypes.h"
#include "mle/utils/File.h"
#include "mle/utils/LuaUTUtils.h"

namespace mle {
namespace {
inline void atLuaPanic(sol::optional<std::string> maybe_msg) {
    MLE_C("Lua panic occurred!");
    if (maybe_msg && !maybe_msg.value().empty()) {
        MLE_C("Lua error: {}", maybe_msg.value());
    } else if (!maybe_msg) {
        MLE_C("Lua provided no error message.");
    }
    Core::i().unrecoverable("Lua panic occurred! aborting application.");
}
}  // namespace

void Lua::init() {
    MLE_I("Creating Lua instance");

    sol_.set_panic(sol::c_call<decltype(&atLuaPanic), &atLuaPanic>);

    sol_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::jit, sol::lib::math, sol::lib::string, sol::lib::table);

    if (!sol_["jit"].valid()) {
        Core::i().unrecoverable("Lua JIT is not available!");
    }

    auto package_path = sol_["package"]["path"];
    package_path = package_path.get<std::string>() + ";res/lua/?.lua";

    MLE_D("{}", sol_["_VERSION"].get<std::string>());
    MLE_D("{}", sol_["jit"]["version"].get<std::string>());
    MLE_D("{}", sol_["package"]["path"].get<std::string>());

    setGlobal("Utils", require("mle/utils"));

    lua::makeUTMathTypes(*this);
    lua::makeUTStopwatch(*this);
    lua::makeUTColor(*this);
}

sol::object Lua::require(const std::string& module_name) {
    MLE_ASSERT_LOG(!module_name.empty(), "Module name must not be empty");
    MLE_ASSERT_LOG(!module_name.ends_with(".lua"), "Module name must not end with .lua");

    MLE_D("Requiring Lua module: {}", module_name);

    return sol_.script_file("res/lua/" + module_name + ".lua");
}

Expected<sol::object> Lua::tryRequire(const std::string& module_name) {
    MLE_ASSERT_LOG(!module_name.empty(), "Module name must not be empty");
    MLE_ASSERT_LOG(!module_name.ends_with(".lua"), "Module name must not end with .lua");

    MLE_D("Trying to require Lua module: {}", module_name);

    Path path = "res/lua/" + module_name + ".lua";
    if (!std::filesystem::exists(path)) {
        MLE_D("Lua module '{}' does not exist at path '{}'", module_name, path);
        return std::unexpected(Result::NOT_FOUND);
    }

    return sol_.script_file(path.string());
};

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

sol::table Lua::mergeTablesNew(const sol::table& a, const sol::table& b) {
    sol::table result = createTable(a, true);
    mergeTables(result, b);
    return result;
}
}  // namespace mle
