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

    kl_.at(0) = std::make_unique<KeyListener>([]() { MLE_W("A key pressed"); }, Key::A, KeyState::PRESSED);
    kl_.at(1) = std::make_unique<KeyListener>([]() { MLE_W("A key released"); }, Key::A, KeyState::RELEASED);
    kl_.at(2) = std::make_unique<KeyListener>([]() { MLE_W("Mouse Left pressed"); }, Key::MOUSE_LEFT, KeyState::PRESSED);
    kl_.at(3) = std::make_unique<KeyListener>([]() { MLE_W("D up, NONE"); }, Key::D, KeyState::UP, KeyModFlagBits::NONE);
    kl_.at(4) = std::make_unique<KeyListener>([]() { MLE_W("E pressed, SHIFT"); }, Key::E, KeyState::PRESSED, KeyModFlagBits::SHIFT);
    kl_.at(5) = std::make_unique<KeyListener>([]() { MLE_W("F released, CTRL"); }, Key::F, KeyState::RELEASED, KeyModFlagBits::CTRL);
    kl_.at(6) = std::make_unique<KeyListener>([]() { MLE_W("G down, ALT"); }, Key::G, KeyState::DOWN, KeyModFlagBits::ALT);
    kl_.at(7) = std::make_unique<KeyListener>([]() { MLE_W("H up, NONE"); }, Key::H, KeyState::UP, KeyModFlagBits::NONE);
    kl_.at(8) = std::make_unique<KeyListener>([]() { MLE_W("I pressed, SHIFT"); }, Key::I, KeyState::PRESSED, KeyModFlagBits::SHIFT);
    kl_.at(9) = std::make_unique<KeyListener>([]() { MLE_W("J released, CTRL"); }, Key::J, KeyState::RELEASED, KeyModFlagBits::CTRL);
    kl_.at(10) = std::make_unique<KeyListener>([]() { MLE_W("K down, ALT"); }, Key::K, KeyState::DOWN, KeyModFlagBits::ALT);
    kl_.at(11) = std::make_unique<KeyListener>([]() { MLE_W("L up, NONE"); }, Key::L, KeyState::UP, KeyModFlagBits::NONE);
    kl_.at(12) = std::make_unique<KeyListener>([]() { MLE_W("M pressed, SHIFT"); }, Key::M, KeyState::PRESSED, KeyModFlagBits::SHIFT);
    kl_.at(13) = std::make_unique<KeyListener>([]() { MLE_W("N released, CTRL"); }, Key::N, KeyState::RELEASED, KeyModFlagBits::CTRL);
    kl_.at(14) = std::make_unique<KeyListener>([]() { MLE_W("O down, ALT"); }, Key::O, KeyState::DOWN, KeyModFlagBits::ALT);
    kl_.at(15) = std::make_unique<KeyListener>([]() { MLE_W("P up, NONE"); }, Key::P, KeyState::UP, KeyModFlagBits::NONE);
    kl_.at(16) = std::make_unique<KeyListener>([]() { MLE_W("Q pressed, SHIFT"); }, Key::Q, KeyState::PRESSED, KeyModFlagBits::SHIFT);
    kl_.at(17) = std::make_unique<KeyListener>([]() { MLE_W("R released, CTRL"); }, Key::R, KeyState::RELEASED, KeyModFlagBits::CTRL);
    kl_.at(18) = std::make_unique<KeyListener>([]() { MLE_W("S down, ALT"); }, Key::S, KeyState::DOWN, KeyModFlagBits::ALT);
    kl_.at(19) = std::make_unique<KeyListener>([]() { MLE_W("T up, NONE"); }, Key::T, KeyState::UP, KeyModFlagBits::NONE);
    kl_.at(20) = std::make_unique<KeyListener>([]() { MLE_W("U pressed, SHIFT"); }, Key::U, KeyState::PRESSED, KeyModFlagBits::SHIFT);
    kl_.at(21) = std::make_unique<KeyListener>([]() { MLE_W("V released, CTRL"); }, Key::V, KeyState::RELEASED, KeyModFlagBits::CTRL);
    kl_.at(22) = std::make_unique<KeyListener>([]() { MLE_W("W down, ALT"); }, Key::W, KeyState::DOWN, KeyModFlagBits::ALT);
    kl_.at(23) = std::make_unique<KeyListener>([]() { MLE_W("X up, NONE"); }, Key::X, KeyState::UP, KeyModFlagBits::NONE);
    kl_.at(24) = std::make_unique<KeyListener>([]() { MLE_W("Y pressed, SHIFT"); }, Key::Y, KeyState::PRESSED, KeyModFlagBits::SHIFT);
    kl_.at(25) = std::make_unique<KeyListener>([]() { MLE_W("Z released, CTRL"); }, Key::Z, KeyState::RELEASED, KeyModFlagBits::CTRL);
    kl_.at(26) = std::make_unique<KeyListener>([]() { MLE_W("ONE down, ALT"); }, Key::ONE, KeyState::DOWN, KeyModFlagBits::ALT);
    kl_.at(27) = std::make_unique<KeyListener>([]() { MLE_W("TWO up, NONE"); }, Key::TWO, KeyState::UP, KeyModFlagBits::NONE);
    kl_.at(28) = std::make_unique<KeyListener>([]() { MLE_W("THREE pressed, SHIFT"); }, Key::THREE, KeyState::PRESSED, KeyModFlagBits::SHIFT);
    kl_.at(29) = std::make_unique<KeyListener>([]() { MLE_W("FOUR released, CTRL"); }, Key::FOUR, KeyState::RELEASED, KeyModFlagBits::CTRL);
    kl_.at(30) = std::make_unique<KeyListener>([]() { MLE_W("FIVE down, ALT"); }, Key::FIVE, KeyState::DOWN, KeyModFlagBits::ALT);
    kl_.at(31) = std::make_unique<KeyListener>([]() { MLE_W("SIX up, NONE"); }, Key::SIX, KeyState::UP, KeyModFlagBits::NONE);
    kl_.at(32) = std::make_unique<KeyListener>([]() { MLE_W("F1 pressed, SHIFT"); }, Key::F1, KeyState::PRESSED, KeyModFlagBits::SHIFT);
    kl_.at(33) = std::make_unique<KeyListener>([]() { MLE_W("F2 released, CTRL"); }, Key::F2, KeyState::RELEASED, KeyModFlagBits::CTRL);
    kl_.at(34) = std::make_unique<KeyListener>([]() { MLE_W("F3 down, ALT"); }, Key::F3, KeyState::DOWN, KeyModFlagBits::ALT);
    kl_.at(35) = std::make_unique<KeyListener>([]() { MLE_W("F4 up, NONE"); }, Key::F4, KeyState::UP, KeyModFlagBits::NONE);
    kl_.at(36) = std::make_unique<KeyListener>([]() { MLE_W("MOUSE_LEFT pressed, SHIFT"); }, Key::MOUSE_LEFT, KeyState::PRESSED, KeyModFlagBits::SHIFT);
    kl_.at(37) = std::make_unique<KeyListener>([]() { MLE_W("MOUSE_RIGHT released, CTRL"); }, Key::MOUSE_RIGHT, KeyState::RELEASED, KeyModFlagBits::CTRL);
    kl_.at(38) = std::make_unique<KeyListener>([]() { MLE_W("MOUSE_MIDDLE down, ALT"); }, Key::MOUSE_MIDDLE, KeyState::DOWN, KeyModFlagBits::ALT);
    kl_.at(39) = std::make_unique<KeyListener>([]() { MLE_W("MOUSE_FOUR up, NONE"); }, Key::MOUSE_FOUR, KeyState::UP, KeyModFlagBits::NONE);
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
    UserInputManager::i().update();

    // FIXME: remove this
    std::this_thread::sleep_for(1ms);

    UserInputManager::i().lateUpdate();

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
