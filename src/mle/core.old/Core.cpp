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
    void setNextScene(std::unique_ptr<Scene>&& scene) { next_scene_ = std::move(scene); }
    auto& threadPool() { return thread_pool_; }

    auto& getLua() { return lua_; }
    auto& getCTable() { return c_table_; }

    void callOnShutdown(std::move_only_function<void()> func) { on_shutdown_funcs_.emplace_back(std::move(func)); }

  private:
    void update();
    void render(f64 dt);
    void updateSecondTimes(std::chrono::seconds current);
    static void logInitParams(const CI& ci);

    void swapScenes();

  private:
    window::WindowCloseListener window_close_listener_;

    ThreadPool thread_pool_;

    std::vector<std::move_only_function<void()>> on_shutdown_funcs_;
};

void Impl::update() {
    swapScenes();

    Stopwatch sw;

    update_call_count_++;

    MLE_D("----- UPDATE ({}) -----", update_call_count_);
    c_table_["time"] = running_time_float_ = running_stopwatch_.elapsedSecFloat();

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

void Impl::shutdown() {
    MLE_I("MLE Core shutting down after {}s", seconds_running_.count());

    renderer::detail::waitIdle();

    for (auto& func : on_shutdown_funcs_) {
        func();
    }
    on_shutdown_funcs_.clear();

    scene_->shutdown();

    thread_pool_.shutdown();  // TODO: notify this needs to stop and kill it first

    audio::shutdown();
    ui::shutdown();
    renderer::shutdown();
    window::shutdown();

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
    lua_.init();

    if (ci.registerClientLuaTypes) {
        MLE_T("USER");
        ci.registerClientLuaTypes();
    }

    c_table_ = lua_.createTable("C");
    c_table_["requestCoreStop"] = []() {
        MLE_VI("Stop requested from lua.");
        core::stop();
    };

    // TODO: remove this requirement
    if (!fs::exists("res/lua/i/config.lua")) {
        core::unrecoverable("App lua config file not found! Expected at: res/lua/i/config.lua");
    }
    sol::table init_config = lua_.require("i/config");

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
