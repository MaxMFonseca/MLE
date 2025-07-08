#pragma once

#include <random>

#include "Client.h"
#include "mle/common/Stopwatch.h"
#include "mle/common/Utils.h"

// Oh my... I will have to deal with encription eventually
// For now lets be NAIVE intentionally
namespace mle::game {
template <typename EventVariant>
class Server {
  public:
    enum class State : u8 { UNINITIALIZED, INITIALIZING, INITIALIZED, RUNNING, STOPPING, STOPPED, SHUTDOWN };

    struct CreateInfo {
        fs::path save_path;
    };

    using CI = CreateInfo;

  public:
    MLE_NO_COPY_MOVE(Server)

    Server() = default;
    virtual ~Server() = default;

    void run() {
        MLE_I("************** SERVER RUN ***************");

        running_stopwatch_.reset();
        auto& sw = running_stopwatch_;
        auto next_update = sw.elapsed<std::chrono::nanoseconds>();

        state_ = State::RUNNING;
        while (state_ == State::RUNNING) {
            auto now = sw.elapsed<std::chrono::nanoseconds>();
            while (now >= next_update) {
                poolEventsExternal();
                update();
                flushEventsExternal();

                now = sw.elapsed<std::chrono::nanoseconds>();
                next_update += TICK_RATE;
            }
        }

        state_ = State::SHUTDOWN;
        shutdown();
    }

    void stop() {
        MLE_I("Server Stop called!");
        state_ = State::STOPPING;
    }

    virtual void init();
    virtual void update();
    virtual void shutdown();

    void pushEvent(EventVariant&& event);
    void poolEventsLocal() {}

  private:
    void poolEventsExternal() {}
    void flushEventsExternal() {}

  private:
    State state_ = State::UNINITIALIZED;
    Stopwatch running_stopwatch_;
    f32 running_time_float_ = 0.F;
    std::chrono::seconds seconds_running_ = 0s;
    i32 update_call_count_ = 0;

    std::mt19937_64 rng_{std::random_device{}()};
};

}  // namespace mle::game
