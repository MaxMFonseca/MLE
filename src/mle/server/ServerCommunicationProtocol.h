#pragma once

#include "mle/utils/Utils.h"
#include "mle/utils/containers/AtomicQueue.h"
namespace mle {

template <typename ServerEvents, typename ClientCommands>
class ServerCommunicationProtocol {
  public:
    MLE_NO_COPY_MOVE(ServerCommunicationProtocol);

    ServerCommunicationProtocol() = default;
    virtual ~ServerCommunicationProtocol() = default;

    virtual void pushEvent(const ServerEvents& ev) = 0;
    virtual void pushEvent(ServerEvents&& ev) = 0;
    virtual bool tryPopEvent(ServerEvents& ev) = 0;

    virtual void pushCommand(const ClientCommands& cmd) = 0;
    virtual void pushCommand(ClientCommands&& cmd) = 0;
    virtual bool tryPopCommand(ClientCommands& cmd) = 0;

    virtual void pump() = 0;

  private:
};

template <typename ServerEvents, typename ClientCommands>
class LocalServerCommunicationProtocol : public ServerCommunicationProtocol<ServerEvents, ClientCommands> {
    MLE_NO_COPY_MOVE(LocalServerCommunicationProtocol);

  public:
    LocalServerCommunicationProtocol() = default;
    ~LocalServerCommunicationProtocol() override = default;

    void pushEvent(const ServerEvents& ev) override { event_queue_.tryPush(ev); }
    void pushEvent(ServerEvents&& ev) override { event_queue_.tryPush(std::move(ev)); }
    bool tryPopEvent(ServerEvents& ev) override { return event_queue_.tryPop(ev); }

    void pushCommand(const ClientCommands& cmd) override { command_queue_.tryPush(cmd); }
    void pushCommand(ClientCommands&& cmd) override { command_queue_.tryPush(std::move(cmd)); }
    bool tryPopCommand(ClientCommands& cmd) override { return command_queue_.tryPop(cmd); }

    void pump() override { MLE_NOOP; }

  public:
    AtomicQueue<ServerEvents> event_queue_{100};
    AtomicQueue<ClientCommands> command_queue_{100};
};
}  // namespace mle
