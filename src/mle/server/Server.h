#pragma once

#include <variant>

#include "mle/core/PerfTracker.h"
#include "mle/server/ServerCommunicationProtocol.h"
#include "mle/utils/Stopwatch.h"
#include "mle/utils/SystemState.h"

// TODO: figure out multi-client handling

namespace mle {
template <typename ServerEvents, typename ClientCommands>
class Server {
  public:
    using SCP = ServerCommunicationProtocol<ServerEvents, ClientCommands>;

  public:
    MLE_NO_COPY_MOVE(Server);

    explicit Server(SCP& communication_protocol) :
        scp_(communication_protocol) {}
    virtual ~Server() = default;

    virtual void init() = 0;

    void requestStop() {
        MLE_I("Server Stop requested!");
        state_.store(SystemState::STOPPING);
    }

    [[nodiscard]] SystemState getState() const { return state_.load(); }

    [[nodiscard]] const auto& getRunningStopwatch() const { return running_sw_; }

  protected:
    virtual void shutdown() { MLE_I("Server shut down successfully after {}s", running_sw_.elapsedSecFloat()); }

    void pushEvent(ServerEvents event) { scp_.pushEvent(std::move(event)); }

    virtual void update() = 0;

    void run() {
        state_.store(SystemState::RUNNING);
        run_thread_ = std::jthread([this]() {
            MLE_I("Server run thread started.");
            runLoop();
        });
    }

    template <typename CommandType>
    void handleCommand(const CommandType& /*cmd*/) {
        MLE_W("Unhandled server command received");
    }

  private:
    void runLoop() {
        MLE_I("Server starting main loop...");

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

            if (accumulator < FIXED_DT) {
                ns sleep_time = FIXED_DT - accumulator;
                if (sleep_time > SLEEP_GUARD) {
                    std::this_thread::sleep_for(sleep_time - SLEEP_GUARD);
                }
            }
        }

        MLE_I("Server stopping...");
        shutdown();
    }

  protected:
    SCP& scp_;

  private:
    Stopwatch running_sw_{};

    std::atomic<SystemState> state_ = SystemState::UNINITIALIZED;
    std::jthread run_thread_{};
};
}  // namespace mle
