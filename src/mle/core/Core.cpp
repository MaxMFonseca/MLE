#include "mle/core/Core.h"

#include "mle/common/Assert.h"
#include "mle/common/Color.h"
#include "mle/common/Consts.h"
#include "mle/common/Stopwatch.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types2D.h"
#include "mle/lua/Lua.h"
// #include "mle/renderer/Renderer.h"
// #include "mle/ui/Controller.h"
// #include "mle/ui/UI.h"
#include "mle/renderer/Renderer.h"
#include "mle/window/Events.h"
#include "mle/window/Window.h"

namespace mle::core {
namespace {
class Impl {
  public:
    void init(CI ci);
    void run();
    void stop();
    void shutdown();
    void shutdownImediate(const std::string& msg);
    void accumulateKPI(SecondKPIType kpi, std::chrono::nanoseconds value);
    [[nodiscard]] std::chrono::milliseconds getRunningTimeMS() const;
    [[nodiscard]] f32 getRunningTimeFloat() const;

  private:
    void update();
    void render();
    void updateSecondTimes(std::chrono::seconds current);
    static void registerLuaTypes(const CI& ci);
    static void registerLuaTypesMath();
    static void logInitParams(const CI& ci);

  private:
    State state_ = State::UNINITIALIZED;
    sol::table mle_table_;

    Stopwatch running_stopwatch_;
    f32 running_time_float_ = 0.F;
    std::chrono::seconds seconds_running_ = 0s;
    i32 update_call_count_ = 0;
    [[maybe_unused]] i32 render_call_count_ = 0;

    window::ED::ListenerHnd<window::events::WindowClose> window_close_listener_;

    struct {
        i32 updates = 0;
        i32 frames = 0;

        std::chrono::nanoseconds time_updating = 0ns;
        std::chrono::nanoseconds time_rendering = 0ns;

        std::array<std::chrono::nanoseconds, static_cast<usize>(SecondKPIType::COUNT)> other_kpi = {};
    } current_second_times_;
};

void Impl::updateSecondTimes(std::chrono::seconds current) {
    MLE_I("second: {}, ups: {}({:.3f}ms) | fps: {}({:.3f}ms)", seconds_running_.count(), current_second_times_.updates,
          current_second_times_.time_updating.count() / (f32)current_second_times_.updates / 1'000'000.F, current_second_times_.frames,
          current_second_times_.time_rendering.count() / (f32)current_second_times_.frames / 1'000'000.F);

    for (int i = 0; i < static_cast<int>(current_second_times_.other_kpi.size()); ++i) {
        MLE_D("{}: {:.3f}ms", (SecondKPIType)i, current_second_times_.other_kpi.at(i).count() / 1'000'000.F);
        current_second_times_.other_kpi.at(i) = 0ns;
    }

    seconds_running_ = current;
    current_second_times_.updates = 0;
    current_second_times_.time_updating = 0ns;
    current_second_times_.frames = 0;
    current_second_times_.time_rendering = 0ns;
}

void Impl::update() {
    Stopwatch sw;

    update_call_count_++;
    MLE_D("----- UPDATE ({}) -----", update_call_count_);

    mle_table_["time"] = running_time_float_ = running_stopwatch_.elapsedSecFloat();

    window::update();
    // ui::update();

    current_second_times_.updates++;
    current_second_times_.time_updating += sw.elapsed<std::chrono::nanoseconds>();
}

void Impl::render() {
    Stopwatch sw;

    render_call_count_++;
    // MLE_D("----- RENDER ({}) -----", render_call_count_);

    if (renderer::beginFrame() == Result::FRAME_SKIP) {
        MLE_I("Frame skipped!");
        return;
    }
    renderer::endFrame(nullptr);

    // ui::render();

    current_second_times_.frames++;
    current_second_times_.time_rendering += sw.elapsed<std::chrono::nanoseconds>();
}

void Impl::registerLuaTypesMath() {
    lua::newUsertype<vec2i>("vec2i", sol::constructors<vec2i(i32, i32)>(), "x", &vec2i::x, "y", &vec2i::y);
    lua::newUsertype<vec3i>("vec3i", sol::constructors<vec3i(i32, i32, i32)>(), "x", &vec3i::x, "y", &vec3i::y, "z", &vec3i::z);
    lua::newUsertype<vec4i>("vec4i", sol::constructors<vec4i(i32, i32, i32, i32)>(), "x", &vec4i::x, "y", &vec4i::y, "z", &vec4i::z, "w", &vec4i::w);

    lua::newUsertype<vec2f>("vec2f", sol::constructors<vec2f(f32, f32)>(), "x", &vec2f::x, "y", &vec2f::y);
    lua::newUsertype<vec3f>("vec3f", sol::constructors<vec3f(f32, f32, f32)>(), "x", &vec3f::x, "y", &vec3f::y, "z", &vec3f::z);
    lua::newUsertype<vec4f>("vec4f", sol::constructors<vec4f(f32, f32, f32, f32)>(), "x", &vec4f::x, "y", &vec4f::y, "z", &vec4f::z, "w", &vec4f::w);

    lua::newUsertype<Rectf>("rectf", sol::constructors<Rectf(f32, f32, f32, f32), Rectf(vec2f, vec2f)>(), "pos", &Rectf::pos, "size", &Rectf::size);
}

void Impl::registerLuaTypes(const CI& ci) {
    MLE_I("Registering Lua types");
    MLE_T("Common");
    registerLuaTypesMath();
    Color::registerLuaTypes();
    MLE_T("UI");
    // ui::registerLuaTypes();
    if (ci.registerLuaTypes) {
        MLE_T("USER");
        ci.registerLuaTypes();
    }
}

void Impl::shutdown() {
    MLE_I("Shutting down MLE Core");
    // renderer::waitIdle();
    MLE_I("MLE Core shutting down after {}s", seconds_running_.count());

    // ui::shutdown();
    renderer::shutdown();
    window::shutdown();
    mle_table_.reset();
    lua::shutdown();

    state_ = State::SHUTDOWN;
    MLE_I("MLE Core shut down successfully");
}

void Impl::shutdownImediate(const std::string& msg) {
    MLE_C("Unrecoverable error: {}", msg);
    MLE_C("Shutting down MLE Core immediately!");
    state_ = State::SHUTDOWN;
    // ui::shutdown();
    // renderer::criticalShutdown();
    // window::criticalShutdown();
    // mle_table_.reset();
    // lua::shutdownImediate(msg);

    std::exit(EXIT_FAILURE);  // NOLINT
}

void Impl::logInitParams(const CI& ci) {
    MLE_I("Init params:");
    MLE_I("  app_name: {}", ci.app_name);
}

void Impl::init(CI ci) {  // NOLINT
    logger::init();
    MLE_I("Initializing MLE Core");
    state_ = State::INITIALIZING;

    logInitParams(ci);
    MLE_T("Lua");
    lua::init();
    registerLuaTypes(ci);

    MLE_ASSERT_LOG(fs::exists(res::addUserLuaPath("config.lua")), "Create the config.lua file inside {}/{}", RES_BASE_PATH, ResPath::LUA);
    sol::table init_config = lua::require("config");

    mle_table_ = lua::createTable("mle");
    Color::addEngineDefaultColors();
    if (const auto colors = init_config["colors"]; colors.valid()) {
        Color::addColors(colors);
    }

    MLE_T("Window");
    window::init({.table = init_config["window"]});
    window_close_listener_ = window::getED().makeEventListener<window::events::WindowClose>([this](const auto&) { stop(); });

    MLE_T("Renderer");
    renderer::init({});

    MLE_T("UI");
    // ui::init({.table = init_config, .init_controller = std::move(ci.init_controller)});

    state_ = State::INITIALIZED;
    MLE_I("Core initialized successfully!");
}

void Impl::run() {
    MLE_I("--------------------------- RUN ---------------------------");
    // ui::switchController();

    running_stopwatch_.reset();
    auto& sw = running_stopwatch_;
    auto next_update = sw.elapsed<std::chrono::nanoseconds>();

    state_ = State::RUNNING;
    while (state_ == State::RUNNING) {
        auto now = sw.elapsed<std::chrono::nanoseconds>();
        while (now >= next_update) {
            next_update += TICK_RATE;
            update();
            now = sw.elapsed<std::chrono::nanoseconds>();
        }

        render();

        auto sec = sw.elapsed<std::chrono::seconds>();
        if (sec > seconds_running_) {
            updateSecondTimes(sec);
        }
    }

    shutdown();
}

void Impl::stop() {
    MLE_I("Core Stop called!");
    state_ = State::STOPPING;
}

void Impl::accumulateKPI(SecondKPIType kpi, std::chrono::nanoseconds value) {
    current_second_times_.other_kpi.at(static_cast<u32>(kpi)) += value;
}

std::chrono::milliseconds Impl::getRunningTimeMS() const {
    return running_stopwatch_.elapsed<std::chrono::milliseconds>();
}

f32 Impl::getRunningTimeFloat() const {
    return running_time_float_;
}

// TODO: I will probably allocate this at a linear allocator along the other core singletons in the future
std::unique_ptr<Impl> i_;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
}  // namespace

void init(CI ci) {
    MLE_ASSERT(!i_);
    i_ = std::make_unique<Impl>();
    i_->init(std::move(ci));
}

void run() {
    MLE_ASSERT(i_);
    i_->run();
}

void stop() {
    MLE_ASSERT(i_);
    i_->stop();
}

void accumulateKPI(SecondKPIType kpi, std::chrono::nanoseconds value) {
    MLE_ASSERT(i_);
    i_->accumulateKPI(kpi, value);
}

std::chrono::milliseconds getRunningTimeMS() {
    MLE_ASSERT(i_);
    return i_->getRunningTimeMS();
}

f32 getRunningTimeF32() {
    MLE_ASSERT(i_);
    return i_->getRunningTimeFloat();
}

void unrecoverable(const std::string& msg) {
    MLE_ASSERT(i_);
    i_->shutdownImediate(msg);
}

}  // namespace mle::core
