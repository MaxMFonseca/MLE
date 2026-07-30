#pragma once

#include <functional>
#include <mutex>
#include <utility>

#include "mle/audio/Types.h"
#include "mle/utils/containers/AtomicQueue.h"

namespace mle::audio {
enum class CommandSubmitResult : u8 {
    ACCEPTED,
    FULL,
    CLOSED,
};

class CommandMailbox {
  public:
    explicit CommandMailbox(usize capacity);

    CommandSubmitResult tryPush(const Cmd& cmd);
    bool tryPop(Cmd& cmd);
    void open();
    void close();
    void reset();
    [[nodiscard]] bool empty() const;

    template <typename Process>
    void drain(Process&& process) {
        auto&& callable = std::forward<Process>(process);
        Cmd cmd;
        while (tryPop(cmd)) {
            std::invoke(callable, cmd);
        }
    }

  private:
    mutable std::mutex producer_mutex_;
    bool accepting_{false};
    AtomicQueue<Cmd> queue_;
};

template <typename Process, typename Teardown>
void drainBeforeTeardown(CommandMailbox& mailbox, Process&& process, Teardown&& teardown) {
    mailbox.drain(std::forward<Process>(process));
    std::invoke(std::forward<Teardown>(teardown));
}
}  // namespace mle::audio
