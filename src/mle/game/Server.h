#pragma once

#include <random>

#include "mle/common/Stopwatch.h"
#include "mle/common/Utils.h"
#include "mle/common/containers/TSQueue.h"
#include "mle/common/math/Types.h"
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

    struct UpdateCtx {
        f32 time{};
        std::vector<server_out_events::Variant> events;
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
        reg_.emplace<server_comp::Transform>(e);
        uctx_.events.emplace_back(server_out_events::NewEntt{e});
        players_.emplace_back(Player{e, true});
        return e;
    }

    void moveEntity(entt::entity e, vec3f v) {
        auto& transform = reg_.get<server_comp::Transform>(e);
        transform.pos += v;
        server_out_events::EnttTransform event;
        event.e = e;
        event.pos = transform.pos;
        uctx_.events.emplace_back(event);
    }

    void transformEntity(entt::entity e, vec3f pos, vec3f scale, f32 rot) {
        auto& transform = reg_.get<server_comp::Transform>(e);
        transform.pos = pos;
        transform.scale = scale;
        transform.rotation = rot;

        server_out_events::EnttTransform event;
        event.e = e;
        event.pos = pos;
        uctx_.events.emplace_back(event);
    }

    void setEntityModel(entt::entity e, const std::string& model_string) {
        reg_.emplace_or_replace<server_comp::Model>(e, model_string);

        server_out_events::EnttModel event;
        event.e = e;
        event.model_string = model_string;
        uctx_.events.emplace_back(event);
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
