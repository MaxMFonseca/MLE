#pragma once

#include <chrono>
#include <variant>

#include "mle/server/ServerCommunicationProtocol.h"
#include "mle/utils/Stopwatch.h"

namespace mle {
template <typename ServerEvents, typename ClientCommands>
class ServerConnection {
  public:
    using SCP = ServerCommunicationProtocol<ServerEvents, ClientCommands>;

  public:
    MLE_NO_COPY_MOVE(ServerConnection);

    explicit ServerConnection(SCP& communication_protocol) :
        scp_(communication_protocol) {}
    virtual ~ServerConnection() = default;

    void pushCmd(const ClientCommands& cmd) { scp_.pushCommand(cmd); }

    void pump() {
        scp_.pump();

        ClientCommands cmd;
        while (scp_.tryPopCommand(cmd)) {
            std::visit([this](const auto& cmd) { handleCommand(std::forward<decltype(cmd)>(cmd)); }, cmd);
        }
    }

    template <typename EvT>
    Expected<EvT> waitForEvent(std::chrono::milliseconds timeout) {
        Stopwatch sw;

        ServerEvents ev;
        while (true) {
            scp_.pump();

            if (scp_.tryPopEvent(ev)) {
                if (auto* ev_ptr = std::get_if<EvT>(&ev)) {
                    return *ev_ptr;
                }
                MLE_W("Received unexpected event type while waiting");
            }

            if (sw.elapsed<std::chrono::milliseconds>() >= timeout) {
                return std::unexpected(Result::TIMEOUT);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    template <typename CmdT>
    void handleCommand(const CmdT& /*cmd*/) {
        MLE_W("Unhandled server command received");
    }

  private:
    SCP& scp_;
};
}  // namespace mle
