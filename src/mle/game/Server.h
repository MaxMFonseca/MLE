#pragma once

#include <random>

#include "mle/common/Stopwatch.h"
#include "mle/common/Utils.h"
#include "mle/common/containers/TSQueue.h"
#include "mle/common/math/Types.h"
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

    struct UpdateCtx {
        f32 time{};
        std::vector<out_ev::Variant> events;
    };

    struct Player {
        entt::entity entity{};
        bool is_local = true;
        // TODO: find a way to not send unecessary/risky information to every player f32 interest_radius = 300.0;
    };

  public:
    MLE_NO_COPY_MOVE(Server)

    Server() = default;
    virtual ~Server() = default;

    virtual entt::entity init(const CI& ci) = 0;

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

    void pushEventLocal(const ServerInEventVariant& event) { local_in_events_.push(event); }

    auto poolPackagesLocal() { return local_ready_packages_.popAll(); }

  protected:
    auto poolLocalInEvents() { return local_in_events_.popAll(); }

    // TODO: external connections

    void initEnttReactive() {
        reg_.on_construct<entt::entity>().connect<&Server::enttCreatedListener>(this);
        reg_.on_destroy<entt::entity>().connect<&Server::enttDestroyedListener>(this);

        reg_.on_construct<secs::Transform>().connect<&Server::enttTransformUpdated>(this);
        reg_.on_update<secs::Transform>().connect<&Server::enttTransformUpdated>(this);

        reg_.on_construct<secs::Model>().connect<&Server::enttModelUpdated>(this);
        reg_.on_update<secs::Model>().connect<&Server::enttModelUpdated>(this);
    }

    void enttCreatedListener(entt::registry& /*unused*/, entt::entity e) {
        MLE_T("Entity created: ", e);
        uctx_.events.emplace_back(out_ev::EnttCreated{e});
    }

    void enttDestroyedListener(entt::registry& /*unused*/, entt::entity e) {
        MLE_T("Entity destroyed: ", e);
        uctx_.events.emplace_back(out_ev::EnttDestroyed{e});
    }

    void enttTransformUpdated(entt::registry& reg, entt::entity e) {
        MLE_T("Entity transform created: ", e);
        auto& comp = reg.get<secs::Transform>(e);
        uctx_.events.emplace_back(out_ev::EnttTransform{.pos = comp.pos, .scale = comp.scale, .rotation = comp.rotation, .e = e});
    }

    void enttModelUpdated(entt::registry& reg, entt::entity e) {
        MLE_T("Entity model updated: ", e);
        auto& comp = reg.get<secs::Model>(e);
        uctx_.events.emplace_back(out_ev::EnttModel{.e = e, .model_string = comp.model_string});
    }

  private:
    virtual void update() = 0;
    virtual void shutdown() = 0;

  private:
    // void poolEventsExternal() {}
    // void flushEventsExternal() {}

    void createOutPackages() {
        for (auto& player : players_) {
            if (!player.is_local) {
                MLE_TODO;
            }

            ServerOutPackage out_data;
            out_data.time_s = uctx_.time;
            out_data.events = std::move(uctx_.events);
            local_ready_packages_.push(std::move(out_data));
        }
    }

  protected:
    UpdateCtx uctx_;

    entt::registry reg_;

    entt::entity createPlayer() {
        auto e = reg_.create();
        players_.emplace_back(Player{e, true});
        return e;
    }
    std::vector<Player> players_;

    TSQueue<ServerOutPackage> local_ready_packages_;
    // map external conections

  private:
    TSQueue<ServerInEventVariant> local_in_events_;
    // TODO: external connections

    State state_ = State::UNINITIALIZED;
    Stopwatch running_stopwatch_;

    // f32 running_time_float_ = 0.F;
    // std::chrono::seconds seconds_running_ = 0s;
    // i32 update_call_count_ = 0;
    // std::mt19937_64 rng_{std::random_device{}()};
};

}  // namespace mle::game
