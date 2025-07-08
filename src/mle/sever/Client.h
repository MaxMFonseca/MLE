#pragma once

#include <entt/entt.hpp>

#include "mle/common/Assert.h"
#include "mle/common/Utils.h"
#include "mle/common/containers/TSQueue.h"
#include "mle/core/Core.h"

namespace mle::game {
template <typename ServerT, typename ServerEventVariant>
class Client final {
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

    void enqueueEvent(ServerEventVariant&& event) { send_queue_.push(std::move(event)); }

    void update() {
        // Send all events to the server
        auto queue_proxy = send_queue_.access();
        for (const auto& e : queue_proxy.get()) {
        }
        queue_proxy.clear();

        // Receive events from the server
        for (const auto& pkg : receive_queue_) {
        }
    }

  private:
    void initLocal(const CI& ci) {
        MLE_I("Initializing local client with save path: {}", ci.save_path.generic_string());
        server_ = std::make_unique<ServerT>();
        server_->init(ci.server_ci);
        core::execAsync([this]() { server_.run(); });
    }

    void initHosting(const CI& /*ci*/) { MLE_TODO; }

    void initReceiver(const CI& /*ci*/) { MLE_TODO; }

  private:
    SceneRenderer scene_renderer_{};

    std::unique_ptr<ServerT> server_ = nullptr;
    // TODO: external server connection

    TSQueue<ServerEventVariant> send_queue_{};

    struct ClientPackage {};
    std::vector<ClientPackage> receive_queue_{};
};
}  // namespace mle::game
