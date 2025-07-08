#pragma once

#include <random>

#include "mle/common/Stopwatch.h"
#include "mle/common/Utils.h"
#include "mle/common/containers/TSQueue.h"
#include "mle/game/DefaultSceneRenderer.h"
#include "mle/game/Types.h"

// Oh my... I will have to deal with encription eventually
// For now lets be NAIVE intentionally
namespace mle::game {
template <typename ServerInEventVariant>
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

    void run(const CI& ci) {
        MLE_I("************** SERVER RUN ***************");

        init(ci);

        running_stopwatch_.reset();
        auto& sw = running_stopwatch_;
        auto next_update = sw.elapsed<std::chrono::nanoseconds>();

        state_ = State::RUNNING;
        while (state_ == State::RUNNING) {
            auto now = sw.elapsed<std::chrono::nanoseconds>();
            while (now >= next_update) {
                // poolEventsExternal();
                ServerOutData current_tick_data;
                current_tick_data.time_s = as<f32>(next_update.count()) / 1'000'000'000.F;
                update(current_tick_data);
                ready_ticks_.push(std::move(current_tick_data));
                // flushEventsExternal();

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

    virtual void init(const CI& ci) = 0;
    virtual void update(ServerOutData& out) = 0;
    virtual void shutdown() = 0;

    void pushEvent(const ServerInEventVariant& event) { in_events_.push(event); }

    auto poolEventsLocal() { return ready_ticks_.popAll(); }

  protected:
    entt::registry reg_;

    entt::entity createEntity(ServerOutData& out_data) {
        auto e = reg_.create();
        out_data.events.emplace_back(server_out_events::NewEntt{e});
        return e;
    }

  private:
    // void poolEventsExternal() {}
    // void flushEventsExternal() {}

  private:
    TSQueue<ServerOutData> ready_ticks_;

    TSQueue<ServerInEventVariant> in_events_;

    State state_ = State::UNINITIALIZED;
    Stopwatch running_stopwatch_;

    // f32 running_time_float_ = 0.F;
    // std::chrono::seconds seconds_running_ = 0s;
    // i32 update_call_count_ = 0;
    // std::mt19937_64 rng_{std::random_device{}()};
};

}  // namespace mle::game
