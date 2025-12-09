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

    void pushCmd(ClientCommands&& cmd) { scp_.pushCommand(std::move(cmd)); }

    template <typename EvT>
    Expected<EvT> waitForEvent(std::chrono::milliseconds timeout) {
        Stopwatch sw;

        ServerEvents ev;
        while (true) {
            scp_.pump();

            if (scp_.tryPopEvent(ev)) {
                if (auto* ev_ptr = std::get_if<EvT>(&ev)) {
                    return std::move(*ev_ptr);
                }
                MLE_W("Received unexpected event type while waiting");
            }

            if (sw.elapsed<std::chrono::milliseconds>() >= timeout) {
                return std::unexpected(Result::TIMEOUT);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    Expected<ServerEvents> waitForEvent(std::chrono::milliseconds timeout) {
        Stopwatch sw;

        ServerEvents ev;
        while (true) {
            scp_.pump();

            if (scp_.tryPopEvent(ev)) {
                return ev;
            }

            if (sw.elapsed<std::chrono::milliseconds>() >= timeout) {
                return std::unexpected(Result::TIMEOUT);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

  protected:
    SCP& scp_;
};
}  // namespace mle
