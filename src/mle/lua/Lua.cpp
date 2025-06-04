#include "Lua.h"

#include "Types.h"
#include "mle/common/Assert.h"
#include "mle/common/Logger.h"
#include "mle/common/Utils.h"
#include "mle/core/Core.h"

namespace mle::lua {
namespace {
class Impl {
  public:
    inline void init();
    inline void shutdown();
    inline sol::object scriptFile(const fs::path& file);
    inline sol::object require(const std::string& module_name, bool engine = false);
    inline sol::table getTable(const std::string& name);
    inline sol::table createTable();
    inline sol::table createTable(const std::string& name);
    inline sol::table createTable(const sol::table& table, bool deep = false);

    sol::state& getSol() { return sol_; }

  private:
    sol::state sol_;
};
// TODO: I will probably allocate this at a linear allocator along the other core singletons in the future
std::unique_ptr<Impl> i_;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

inline void atLuaPanic(sol::optional<std::string> maybe_msg) {
    MLE_C("Lua panic occurred!");
    if (maybe_msg && !maybe_msg.value().empty()) {
        MLE_C("Lua error: {}", maybe_msg.value());
    } else if (!maybe_msg) {
        MLE_C("Lua provided no error message.");
    }
    core::unrecoverable("Lua panic occurred! aborting application.");
}

void Impl::init() {
    MLE_I("Initializing Lua Module");

    sol_.set_panic(sol::c_call<decltype(&atLuaPanic), &atLuaPanic>);

    sol_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::jit, sol::lib::math, sol::lib::string, sol::lib::table);

    auto package_path = sol_["package"]["path"];
    package_path = package_path.get<std::string>() + ";" + ResPath::LUA + "?.lua";

    if (!sol_["jit"].valid()) {
        core::unrecoverable("Lua JIT is not available!");
    }

    MLE_D("{}", sol_["_VERSION"].get<std::string>());
    MLE_D("{}", sol_["jit"]["version"].get<std::string>());

    require("utils", true);
    MLE_T("LuaUtils: {}", getTable("LuaUtils"));

    MLE_I("Module initialized successfully!");
}

void Impl::shutdown() {  // NOLINT this can be static for now but should not be static in the future
    MLE_I("Shutting down Lua Module");
    MLE_D("Module shut down successfully!");
}

sol::object Impl::scriptFile(const fs::path& file) {
    return sol_.script_file(file);
}

sol::object Impl::require(const std::string& module_name, bool engine) {
    MLE_D("Requiring Lua module: {}, engine: {}", module_name, engine);
    if (engine) {
        return scriptFile(res::addMleLuaPath(module_name + ".lua"));
    }
    return scriptFile(res::addUserLuaPath(module_name + ".lua"));
}

sol::table Impl::getTable(const std::string& name) {
    return sol_.get<sol::table>(name);
}

sol::table Impl::createTable() {
    return sol_.create_table();
}

sol::table Impl::createTable(const std::string& name) {
    MLE_D("Creating global table: {}", name);
    return sol_.create_named_table(name);
}

sol::table Impl::createTable(const sol::table& table, bool deep) {
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
}  // namespace

void init() {
    MLE_ASSERT(!i_);
    i_ = std::make_unique<Impl>();
    i_->init();
}

void shutdown() {
    if (i_) {
        MLE_I("Shutting down Lua Module");
        i_->shutdown();
        i_.reset();
    }
}

sol::object scriptFile(const fs::path& file) {
    MLE_ASSERT(i_);
    return i_->scriptFile(file);
}

sol::object require(const std::string& module_name, bool engine) {
    MLE_ASSERT(i_);
    return i_->require(module_name, engine);
}

sol::table createTable() {
    MLE_ASSERT(i_);
    return i_->createTable();
}

sol::table createTable(const std::string& name) {
    MLE_ASSERT(i_);
    return i_->createTable(name);
}

sol::table createTable(const sol::table& table, bool deep) {
    MLE_ASSERT(i_);
    return i_->createTable(table, deep);
}

namespace detail {
sol::state& getSol() {
    MLE_ASSERT(i_);
    return i_->getSol();
}
}  // namespace detail
}  // namespace mle::lua
