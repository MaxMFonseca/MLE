#include "Core.h"

#include <chrono>
#include <random>
#include <thread>

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
#include "detail/ThreadPool.h"
#include "mle/audio/Audio.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/UI.h"
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
    void execAsync(std::function<void()>&& func);
    auto& rng() { return rng_; }
    void setNextScene(std::unique_ptr<Scene>&& scene) { next_scene_ = std::move(scene); }

  private:
    void update();
    void render(f64 dt);
    void updateSecondTimes(std::chrono::seconds current);
    static void registerLuaTypes(const CI& ci);
    static void registerLuaTypesMath();
    static void logInitParams(const CI& ci);

    void swapScenes();

  private:
    State state_ = State::UNINITIALIZED;

    Stopwatch running_stopwatch_;
    f32 running_time_float_ = 0.F;
    std::chrono::seconds seconds_running_ = 0s;
    i32 update_call_count_ = 0;
    [[maybe_unused]] i32 render_call_count_ = 0;

    window::WindowCloseListener window_close_listener_;

    struct {
        i32 updates = 0;
        i32 frames = 0;

        std::chrono::nanoseconds time_updating = 0ns;
        std::chrono::nanoseconds time_rendering = 0ns;

        std::array<std::chrono::nanoseconds, static_cast<usize>(SecondKPIType::COUNT)> other_kpi = {};
    } current_second_times_;

    detail::ThreadPool thread_pool_;

    std::mt19937_64 rng_{std::random_device{}()};

    std::unique_ptr<Scene> scene_ = nullptr;
    std::unique_ptr<Scene> next_scene_ = nullptr;
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
    swapScenes();

    Stopwatch sw;

    update_call_count_++;
    MLE_D("----- UPDATE ({}) -----", update_call_count_);

    lua::getMleTable()["time"] = running_time_float_ = running_stopwatch_.elapsedSecFloat();

    window::update();
    ui::update();
    renderer::update();
    audio::update();

    scene_->update();

    window::lateUpdate();

    current_second_times_.updates++;
    current_second_times_.time_updating += sw.elapsed<std::chrono::nanoseconds>();
}

void Impl::swapScenes() {
    if (!next_scene_) {
        return;
    }

    MLE_I("Swapping scenes...");

    renderer::detail::waitIdle();

    if (scene_) {
        scene_->shutdown();
    }
    scene_ = std::move(next_scene_);
    next_scene_ = nullptr;
    scene_->init();
}

void Impl::render(f64 dt) {
    Stopwatch sw;

    render_call_count_++;
    // MLE_D("----- RENDER ({}) -----", render_call_count_);

    if (renderer::beginFrame() == Result::FRAME_SKIP) {
        MLE_I("Frame skipped!");
        return;
    }

    scene_->render(dt);

    renderer::endFrame(ui::render());

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

    MLE_T("Audio");
    audio::registerLuaTypes();

    if (ci.registerLuaTypes) {
        MLE_T("USER");
        ci.registerLuaTypes();
    }

    MLE_T("Core");
    auto core_table = lua::createTable();
    core_table["stop"] = []() {
        MLE_VI("Stop requested from lua.");
        core::stop();
    };

    lua::getMleTable()["core"] = core_table;

    MLE_D("MLE table: {}", lua::getMleTable());
}

void Impl::shutdown() {
    MLE_I("MLE Core shutting down after {}s", seconds_running_.count());

    renderer::detail::waitIdle();

    scene_->shutdown();

    thread_pool_.shutdown();  // TODO: notify this needs to stop and kill it first

    audio::shutdown();
    ui::shutdown();
    renderer::shutdown();
    window::shutdown();
    lua::shutdown();

    state_ = State::UNINITIALIZED;
    MLE_I("MLE Core shut down successfully");
}

void Impl::shutdownImediate(const std::string& msg) {
    MLE_C("Unrecoverable error: {}", msg);
    MLE_C("Shutting down MLE Core immediately!");
    state_ = State::UNINITIALIZED;
    // ui::shutdown();
    // renderer::criticalShutdown();
    // window::criticalShutdown();
    // mle_table_.reset();
    // lua::shutdownImediate(msg);

    MLE_C("PLEASE FIND A WAY TO DO THIS GRACEFULLY!");
    MLE_C("PLEASE FIND A WAY TO DO THIS GRACEFULLY!");
    MLE_C("PLEASE FIND A WAY TO DO THIS GRACEFULLY!");
    MLE_C("PLEASE FIND A WAY TO DO THIS GRACEFULLY!");
    MLE_C("PLEASE FIND A WAY TO DO THIS GRACEFULLY!");

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

    thread_pool_.init();

    thread_pool_.enqueue([]() { MLE_I("MT test A: tid: {}", std::hash<std::thread::id>{}(std::this_thread::get_id())); });  // NOLINT
    thread_pool_.enqueue([]() { MLE_I("MT test B: tid: {}", std::hash<std::thread::id>{}(std::this_thread::get_id())); });  // NOLINT
    thread_pool_.enqueue([]() { MLE_I("MT test C: tid: {}", std::hash<std::thread::id>{}(std::this_thread::get_id())); });  // NOLINT
    thread_pool_.enqueue([]() { MLE_I("MT test D: tid: {}", std::hash<std::thread::id>{}(std::this_thread::get_id())); });  // NOLINT
    thread_pool_.enqueue([]() { MLE_I("MT test E: tid: {}", std::hash<std::thread::id>{}(std::this_thread::get_id())); });  // NOLINT

    logInitParams(ci);
    MLE_T("Lua");
    lua::init();
    registerLuaTypes(ci);

    // TODO: remove this requirement
    if (!fs::exists("res/lua/i/config.lua")) {
        core::unrecoverable("App lua config file not found! Expected at: res/lua/i/config.lua");
    }
    sol::table init_config = lua::require("i/config");

    Color::addEngineDefaultColors();
    if (const auto colors = init_config["colors"]; colors.valid()) {
        Color::addColors(colors);
    }

    MLE_T("Window");
    window::init({.table = init_config["window"]});
    window_close_listener_ = window::listen<window::events::WindowClose>([this](const auto&) { stop(); });

    MLE_T("Renderer");
    renderer::init({});

    MLE_T("UI");
    ui::init();

    MLE_T("Audio");
    audio::init();

    MLE_ASSERT_LOG(ci.scene, "A scene must be provided in CI!");
    setNextScene(std::move(ci.scene));

    state_ = State::INITIALIZED;
    MLE_I("Core initialized successfully!");
}

void Impl::run() {
    MLE_I("--------------------------- RUN ---------------------------");

    running_stopwatch_.reset();
    auto& sw = running_stopwatch_;
    auto next_update = sw.elapsed<std::chrono::nanoseconds>();

    auto last_render = next_update;

    state_ = State::RUNNING;
    while (state_ == State::RUNNING) {
        auto now = sw.elapsed<std::chrono::nanoseconds>();
        while (now >= next_update) {
            next_update += TICK_RATE;
            update();
            now = sw.elapsed<std::chrono::nanoseconds>();
        }

        now = sw.elapsed<std::chrono::nanoseconds>();
        f64 now_f64 = as<f64>(now.count()) * 1e-9;
        f64 last_f64 = as<f64>(last_render.count()) * 1e-9;
        f64 dt = now_f64 - last_f64;
        last_render = now;
        render(dt);

        auto sec = sw.elapsed<std::chrono::seconds>();
        if (sec > seconds_running_) {
            updateSecondTimes(sec);
        }
    }

    state_ = State::SHUTDOWN;
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

void Impl::execAsync(std::function<void()>&& func) {
    thread_pool_.enqueue(std::move(func));
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

void execAsync(std::function<void()>&& func) {
    MLE_ASSERT(i_);
    i_->execAsync(std::move(func));
}

std::mt19937_64& rng() {
    MLE_ASSERT(i_);
    return i_->rng();
}

i64 rngi(i64 max, i64 min) {
    MLE_ASSERT(i_);
    std::uniform_int_distribution<i64> dist(min, max);
    return dist(i_->rng());
}

u64 rngu(u64 max, u64 min) {
    MLE_ASSERT(i_);
    std::uniform_int_distribution<u64> dist(min, max);
    return dist(i_->rng());
}

f32 rngf(f32 max, f32 min) {
    MLE_ASSERT(i_);
    std::uniform_real_distribution<f32> dist(min, max);
    return dist(i_->rng());
}

bool maybe(f32 chance) {
    MLE_ASSERT(i_);
    std::bernoulli_distribution dist(chance);
    return dist(i_->rng());
}

void setNextScene(std::unique_ptr<Scene>&& scene) {
    MLE_ASSERT(i_);
    i_->setNextScene(std::move(scene));
}
}  // namespace mle::core
