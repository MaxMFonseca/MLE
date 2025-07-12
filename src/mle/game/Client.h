#pragma once
// TODO
#include <entt/entt.hpp>

#include "mle/common/Assert.h"
#include "mle/common/EventDispatcher.h"
#include "mle/common/Utils.h"
#include "mle/common/containers/TSQueue.h"
#include "mle/core/Core.h"
#include "mle/game/Types.h"

namespace mle::game {
template <typename ServerInEventVariantT, typename ServerOutEventVariantT, typename ServerT>
class Client {
  public:
    using ServerOutED = EDFromVariant<ServerOutEventVariantT>::Type;

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

    void enqueueServerEvent(ServerInEventVariantT&& event) { send_queue_.push(std::move(event)); }

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
            last_server_time_ = server_time_;
            server_time_ = server_tick.time;
            for (const auto& event : server_tick.events) {
                std::visit([&](const auto& e) { received_ed_.dispatch(e); }, event);
            }
        }
    }

    auto& getReceivedED() { return received_ed_; }

    [[nodiscard]] f32 getLastServerTime() const { return last_server_time_; }
    [[nodiscard]] f32 getServerTime() const { return server_time_; }

    entt::entity getLocalPlayer() { return server_->getLocalPlayer(); }

  private:
    void initLocal(const CI& ci) {
        server_ = std::make_unique<ServerT>();
        server_->init(ci.server_ci);
        MLE_ASSERT_LOG(getLocalPlayer() != entt::null, "server init must set local_player_");
        core::threadPool().enqueue([this]() { server_->run(); });
    }

    void initHosting(const CI& /*ci*/) { MLE_TODO; }

    void initReceiver(const CI& /*ci*/) { MLE_TODO; }

  private:
    std::unique_ptr<ServerT> server_ = nullptr;
    // TODO: external server connection

    TSQueue<ServerInEventVariantT> send_queue_{};

    f32 last_server_time_ = 0;
    f32 server_time_ = 0;
    ServerOutED received_ed_{};
};
}  // namespace mle::game
