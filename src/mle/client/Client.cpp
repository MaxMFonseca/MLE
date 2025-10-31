#include "Client.h"

#include <chrono>

#include "mle/audio/AudioEngine.h"
#include "mle/audio/Types.h"
#include "mle/audio/Utils.h"
#include "mle/client/layers/PerfLayer.h"
#include "mle/client/layers/TerminalLayer.h"
#include "mle/core/Assert.h"
#include "mle/core/Logger.h"
#include "mle/core/PerfTracker.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Renderer.h"
#include "mle/utils/Color.h"
#include "mle/utils/ECS.h"
#include "mle/utils/Stopwatch.h"
#include "mle/window/TextBox.h"
#include "mle/window/Window.h"

namespace mle {
void Client::init() {
    static bool initialized = false;
    MLE_ASSERT_LOG(!initialized, "Client already initialized!");
    initialized = true;

    MLE_I("MLE Client initializing...");

    Color::addEngineDefaultColors();

    MLE_I("Lua init");
    lua_.init();

    MLE_D("Tailwind colors.");
    Color::addColors(lua_.require("mle/tailwind_colors").as<sol::table>());

    MLE_D("Creating client table in Lua");
    client_table_ = lua_.createTable("C");

    MLE_I("Window init");
    Window::i().init();

    MLE_I("AudioEngine init");
    Core::i().check(AudioEngine::i().init(), "Failed to initialize AudioEngine");

    mle::AudioEngine::addLuaBinding();

    window_close_el_ = Window::i().getED().makeListener<window::ev::Close>([this](const auto&) { requestStop(); });

    state_ = SystemState::INITIALIZED;
    MLE_I("MLE Client initialized successfully.");
}

void Client::addPerfLayer() {
    MLE_I("Adding PerfLayer");
    auto r = debug_layers_.emplace("perf", std::make_unique<client::PerfLayer>());
    if (r.second) {
        r.first->second->init();
    }
}

void Client::addTerminalLayer() {
    MLE_I("Adding TerminalLayer");
    auto r = debug_layers_.emplace("terminal", std::make_unique<client::TerminalLayer>());
    if (r.second) {
        r.first->second->init();
    }
}

void Client::run() {
    MLE_I("MLE Client starting main loop...");

    using ns = std::chrono::nanoseconds;
    using namespace std::chrono_literals;

    constexpr auto FIXED_DT = 16'666'667ns;
    constexpr int MAX_CATCH_UP = 5;
    constexpr auto SLEEP_GUARD = 200'000ns;

    state_ = SystemState::RUNNING;

    auto& sw = running_sw_;
    running_sw_.reset();

    auto t_prev = running_sw_.elapsed<ns>();
    std::chrono::nanoseconds accumulator = 0ns;

    Stopwatch test_sw;
    AudioEngine::i().enqueueCmd(audio::cmd::Load{
        .name = "mle/t",
        .stream = false,
    });
    AudioEngine::i().enqueueCmd(audio::cmd::Load{
        .name = "i/ambient",
        .stream = true,
    });
    AudioEngine::i().enqueueCmd(audio::cmd::Load{
        .name = "i/ambient7s",
        .stream = true,
    });

    if (!next_game_layer_) {
        MLE_W("No initial game layer set! Pushing empty layer.");
        pushGameLayer(std::make_unique<client::Layer>());
    }

    addPerfLayer();
    addTerminalLayer();

    while (state_ == SystemState::RUNNING) {
        const auto t_now = sw.elapsed<ns>();
        accumulator += std::chrono::duration_cast<ns>(t_now - t_prev);
        t_prev = t_now;

        int updates = 0;
        while (accumulator >= FIXED_DT && updates < MAX_CATCH_UP) {
            update();
            accumulator -= FIXED_DT;
            ++updates;
        }

        if (updates == MAX_CATCH_UP && accumulator >= FIXED_DT) {
            accumulator %= FIXED_DT;
        }

        client_table_["time"] = running_sw_.elapsedSecFloat();

        // if (test_sw.elapsedMSFloat() > 33) {
        //     test_sw.reset();
        //     AudioEngine::i().enqueueCmd(audio::cmd::PlayOneShot{
        //         .sound_id = entt::hashed_string{"mle/t"},
        //     });
        // }

        // const f64 alpha = static_cast<f64>(accumulator.count()) / static_cast<f64>(FIXED_DT.count());

        if (accumulator < FIXED_DT) {
            ns sleep_time = FIXED_DT - accumulator;
            if (sleep_time > SLEEP_GUARD) {
                std::this_thread::sleep_for(sleep_time - SLEEP_GUARD);
            }
        }
    }

    MLE_I("MLE client stopping...");
    shutdown();
}

void Client::update() {
    MLE_PERF_SCOPE("ClientUpdate");

    checkNextGameLayer();

    Window::i().poolEvents();
    UserInputManager::i().update();

    {
        MLE_PERF_SCOPE("ClientUpdate.GameLayer");
        game_layer_->update();
    }

    {
        MLE_PERF_SCOPE("ClientUpdate.DebugLayers");
        for (auto& [_, dl] : debug_layers_) {
            dl->update();
        }

        if (!debug_layers_to_remove_.empty()) {
            std::scoped_lock lock(debug_layers_render_mutex_);
            for (const auto& name : debug_layers_to_remove_) {
                auto it = debug_layers_.find(name);
                if (it != debug_layers_.end()) {
                    MLE_I("Removing debug layer '{}'", name);
                    debug_layers_.erase(it);
                }
            }
            debug_layers_to_remove_.clear();
        }
    }

    UserInputManager::i().lateUpdate();
}

void Client::shutdown() {
    MLE_I("MLE Client shutting down after {}s", running_sw_.elapsedSecFloat());
    Stopwatch sw;
    AudioEngine::i().shutdown();
    {
        std::scoped_lock lock(game_layer_render_mutex_);
        if (game_layer_) {
            MLE_I("Shutting down current game layer");
            game_layer_->shutdown();
            game_layer_.reset();
        }
    }
    {
        std::scoped_lock lock(debug_layers_render_mutex_);
        for (auto& [_, dl] : debug_layers_) {
            dl->shutdown();
        }
        debug_layers_.clear();
    }
    MLE_I("MLE Client shut down successfully after {}s", sw.elapsedSecFloat());
}

void Client::requestStop() {
    MLE_I("Client Stop requested!");
    state_ = SystemState::STOPPING;
}

void Client::checkNextGameLayer() {
    if (next_game_layer_) {
        std::scoped_lock lock(game_layer_render_mutex_);

        if (game_layer_) {
            MLE_I("Shutting down current game layer");
            game_layer_->shutdown();
            game_layer_.reset();
        }

        MLE_I("Switching to new game layer");
        game_layer_ = std::move(next_game_layer_);
        game_layer_->init();
    }
}

ImageRef Client::render() {
    if (!game_layer_) {
        return nullptr;
    }

    ImageRef game_layer_img = nullptr;

    {
        std::scoped_lock lock(game_layer_render_mutex_);
        game_layer_img = game_layer_->render(0);
    }
    if (!game_layer_img) {
        return nullptr;
    }

    if (!debug_layers_.empty()) {
        std::scoped_lock lock(debug_layers_render_mutex_);
        for (auto& [_, dl] : debug_layers_) {
            dl->renderTo(game_layer_img);
        }
    }

    return game_layer_img;
}

void Client::addDebugLayer(const std::string& name, std::unique_ptr<client::Layer> layer) {
    std::scoped_lock lock(debug_layers_render_mutex_);
    debug_layers_.emplace(name, std::move(layer));
};

void Client::removeDebugLayer(const std::string& name) {
    debug_layers_to_remove_.insert(name);
};
}  // namespace mle
