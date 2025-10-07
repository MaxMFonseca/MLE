#include "Client.h"

#include <chrono>

#include "mle/core/Assert.h"
#include "mle/core/Logger.h"
#include "mle/utils/Stopwatch.h"
#include "mle/window/Window.h"

namespace mle {
void Client::init() {
    static bool initialized = false;
    MLE_ASSERT_LOG(!initialized, "Client already initialized!");
    initialized = true;

    MLE_I("MLE Client initializing...");

    MLE_I("Lua init");
    lua_.init();

    MLE_I("Window init");
    Window::i().init();

    window_close_el_ = Window::i().getED().makeListener<window::ev::Close>([this](const auto&) { requestStop(); });
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

        if (sw.elapsedSecInt() > time_.seconds) {
            updateTimeStatsEachSecond();
        }
    }

    MLE_I("MLE client stopping...");
    shutdown();
}

void Client::update() {
    Stopwatch sw;

    Window::i().poolEvents();

    // FIXME: remove this
    std::this_thread::sleep_for(1ms);

    time_.current_second.updates++;
    time_.current_second.time_updating_ms += sw.elapsedMSFloat();
}

void Client::render(f64 /*unused*/) {
}

void Client::shutdown() {
    MLE_I("MLE Client shutting down after {}s", running_sw_.elapsedSecFloat());
    MLE_I("Time stats at shutdown: {}", time_.all_time);
    Stopwatch sw;
    MLE_I("Calling {} shutdown callbacks", on_shutdown_callbacks_.size());
    state_ = SystemState::UNINITIALIZED;
    MLE_I("MLE Client shut down successfully after {}s", sw.elapsedSecFloat());
}

void Client::requestStop() {
    MLE_I("Client Stop requested!");
    state_ = SystemState::STOPPING;
}

void Client::updateTimeStatsEachSecond() {
    int time_seconds_running_aux = time_.seconds + 1;
    time_.seconds = running_sw_.elapsedSecInt();
    MLE_ASSERT_LOG(time_.seconds == time_seconds_running_aux, "This shouldnt happen! {} != {}", time_.seconds, time_seconds_running_aux);

    time_.last_second = time_.current_second;

    auto& all = time_.all_time;
    all.updates += time_.current_second.updates;
    all.frames += time_.current_second.frames;
    all.time_updating_ms += time_.current_second.time_updating_ms;
    all.time_rendering_ms += time_.current_second.time_rendering_ms;

    for (const auto& [key, value] : time_.current_second.other_times_ms_) {
        all.other_times_ms_[key] += value;
    }

    time_.current_second = {};

    logTimeStatsLastSecond();
    if (time_.seconds % 30 == 0) {
        logTimeStatsAllTime();
    }
}

void Client::logTimeStatsLastSecond() const {
    MLE_I("----- Client Time Stats (Last Second) -----\nnow:{}s\n{}", time_.seconds, time_.last_second);
}

void Client::logTimeStatsAllTime() const {
    MLE_I("----- Client Time Stats (All Time) -----\n now:{}s\n {}", time_.seconds, time_.all_time);

    TimeStats::Tracked t2 = time_.all_time;
    t2.updates /= time_.seconds;
    t2.frames /= time_.seconds;
    t2.time_updating_ms /= as<f32>(time_.seconds);
    t2.time_rendering_ms /= as<f32>(time_.seconds);
    for (auto& [key, value] : t2.other_times_ms_) {
        value /= as<f32>(time_.seconds);
    }
    MLE_I("----- Client Time Stats (All Time Averages) -----\n now:{}s\n {}", time_.seconds, t2);
}
}  // namespace mle
