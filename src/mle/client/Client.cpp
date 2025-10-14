#include "Client.h"

#include <chrono>

#include "mle/core/Assert.h"
#include "mle/core/Logger.h"
#include "mle/core/PerfTracker.h"
#include "mle/utils/Color.h"
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

    MLE_I("Window init");
    Window::i().init();

    window_close_el_ = Window::i().getED().makeListener<window::ev::Close>([this](const auto&) { requestStop(); });

    auto* tb = new TextBox;
    tb->setText(U"Hello, World!");
    tb->setChangedCallback([tb]() { MLE_I("TextBox changed: \n{}\n{}", tb->getTextUtf8(), tb->makeSelectionString()); });
    tb->setFocused(true);

    state_ = SystemState::INITIALIZED;
    MLE_I("MLE Client initialized successfully.");
}

void Client::run() {
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
    MLE_PERF_SCOPE("Client::update");

    Window::i().poolEvents();
    UserInputManager::i().update();

    // FIXME: remove this
    std::this_thread::sleep_for(1ms);

    UserInputManager::i().lateUpdate();
}

void Client::shutdown() {
    MLE_I("MLE Client shutting down after {}s", running_sw_.elapsedSecFloat());
    Stopwatch sw;
    MLE_I("Calling {} shutdown callbacks", on_shutdown_callbacks_.size());
    for (auto& cb : on_shutdown_callbacks_) {
        cb();
    }
    state_ = SystemState::UNINITIALIZED;
    MLE_I("MLE Client shut down successfully after {}s", sw.elapsedSecFloat());
}

void Client::requestStop() {
    MLE_I("Client Stop requested!");
    state_ = SystemState::STOPPING;
}

}  // namespace mle
