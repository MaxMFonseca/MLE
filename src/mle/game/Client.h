#pragma once

#include <entt/entt.hpp>

#include "mle/common/Assert.h"
#include "mle/common/Utils.h"
#include "mle/common/containers/TSQueue.h"
#include "mle/core/Core.h"
#include "mle/game/DefaultSceneRenderer.h"
#include "mle/game/Types.h"

namespace mle::game {
template <typename EventVariantT, typename ServerT>
class Client {
  public:
    enum class ConnectionType : u8 { LOCAL, HOSTING, RECEIVER };

    struct CreateInfo {
        ConnectionType connection_type = ConnectionType::LOCAL;

        ServerT::CI server_ci{};
    };

    using CI = CreateInfo;

  public:
    MLE_NO_COPY_MOVE(Client)

    Client() = default;
    ~Client() = default;

    void init(const CI& ci) {
        switch (ci.connection_type) {
            case ConnectionType::LOCAL:
                initLocal(ci);
                break;
            case ConnectionType::HOSTING:
                initHosting(ci);
                break;
            case ConnectionType::RECEIVER:
                initReceiver(ci);
                break;
        }
    }

    void shutdown() {
        if (server_) {
            server_->stop();
        }
    }

    void enqueueServerEvent(EventVariantT&& event) { send_queue_.push(std::move(event)); }

    void flushEvents() {
        MLE_ASSERT_LOG(server_ != nullptr, "Server is not initialized!");

        for (const auto& e : send_queue_.popAll()) {
            server_->pushEventLocal(e);
        }
    }

    void poolEvents() {
        MLE_ASSERT_LOG(server_ != nullptr, "Server is not initialized!");

        auto server_ticks = server_->poolPackagesLocal();
        for (const auto& server_tick : server_ticks) {
            server_out_events::Time time_event{server_tick.time_s};
            server_out_ed_.dispatch(time_event);
            for (const auto& event : server_tick.events) {
                std::visit([&](const auto& e) { server_out_ed_.dispatch(e); }, event);
            }
        }
    }

    auto& getServerEventED() { return server_out_ed_; }

  private:
    void initLocal(const CI& ci) {
        server_ = std::make_unique<ServerT>();
        core::execAsync([this, ci]() { server_->run(ci.server_ci); });
    }

    void initHosting(const CI& /*ci*/) { MLE_TODO; }

    void initReceiver(const CI& /*ci*/) { MLE_TODO; }

  private:
    std::unique_ptr<ServerT> server_ = nullptr;
    // TODO: external server connection

    TSQueue<EventVariantT> send_queue_{};

    ServerOutED server_out_ed_{};
};
}  // namespace mle::game
