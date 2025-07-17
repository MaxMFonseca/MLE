#pragma once

#include <random>

#include "mle/common/Assert.h"
#include "mle/common/Stopwatch.h"
#include "mle/common/Utils.h"
#include "mle/common/containers/TSQueue.h"
#include "mle/common/math/Types.h"
#include "mle/game/Types.h"
#include "mle/lua/Lua.h"

// Oh my... I will have to deal with encription eventually
// For now lets be NAIVE intentionally
namespace mle::game {
template <typename ServerInEventVariantT, typename ServerOutEventVariantT>
class Server {
  public:
    enum class State : u8 { UNINITIALIZED, INITIALIZING, INITIALIZED, RUNNING, STOPPING, STOPPED, SHUTDOWN };

    struct CreateInfo {
        fs::path save_path;
    };

    using CI = CreateInfo;

    struct UpdateCtx {
        f32 time{};
        std::vector<ServerOutEventVariantT> events;
    };

    struct ServerOutPackage : UpdateCtx {};

    // struct Player {
    //     entt::entity entity{};
    //     conection things
    //     // TODO: find a way to not send unecessary/risky information to every player like: f32 interest_radius = 300.0; flags...
    // };

  public:
    MLE_NO_COPY_MOVE(Server)

    Server() = default;
    virtual ~Server() = default;

    virtual void init(const CI& ci) = 0;

    void run() {
        MLE_I("************** SERVER RUN ***************");

        running_stopwatch_.reset();
        auto& sw = running_stopwatch_;
        auto next_update = sw.elapsed<std::chrono::nanoseconds>();

        state_ = State::RUNNING;
        while (state_ == State::RUNNING) {
            auto now = sw.elapsed<std::chrono::nanoseconds>();
            while (now >= next_update) {
                uctx_.time = as<f32>(next_update.count()) / 1'000'000'000.F;

                update();

                createOutPackages();

                now = sw.elapsed<std::chrono::nanoseconds>();
                next_update += TICK_RATE;

                uctx_ = {};
            }
        }

        state_ = State::SHUTDOWN;
        shutdown();
    }

    void stop() {
        MLE_I("Server Stop called!");
        state_ = State::STOPPING;
    }

    void pushEventLocal(const ServerInEventVariantT& event) { local_in_events_.push(event); }

    auto poolPackagesLocal() { return local_ready_packages_.popAll(); }

    auto getLocalPlayer() const { return local_player_; }

  protected:
    auto poolLocalInEvents() { return local_in_events_.popAll(); }

    // TODO: external connections

  private:
    virtual void update() = 0;
    virtual void shutdown() = 0;

  private:
    // void poolEventsExternal() {}
    // void flushEventsExternal() {}

    void createOutPackages() {
        ServerOutPackage out_data;
        out_data.time = uctx_.time;
        if (uctx_.events.empty()) {
            return;
        }
        out_data.events = std::move(uctx_.events);
        local_ready_packages_.push(std::move(out_data));

        uctx_ = {};

        // TODO: for (auto& player : players_) {
        // }
    }

  protected:
    Lua lua_;

    UpdateCtx uctx_;

    entt::registry reg_;
    entt::entity local_player_{};

    TSQueue<ServerOutPackage> local_ready_packages_;
    // map external conections

  private:
    TSQueue<ServerInEventVariantT> local_in_events_;
    // TODO: external connections

    State state_ = State::UNINITIALIZED;
    Stopwatch running_stopwatch_;

    // f32 running_time_float_ = 0.F;
    // std::chrono::seconds seconds_running_ = 0s;
    // i32 update_call_count_ = 0;
    // std::mt19937_64 rng_{std::random_device{}()};
};

}  // namespace mle::game
