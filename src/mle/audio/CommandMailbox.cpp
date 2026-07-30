#include "mle/audio/CommandMailbox.h"

namespace mle::audio {
CommandMailbox::CommandMailbox(usize capacity) :
    queue_(capacity) {
}

CommandSubmitResult CommandMailbox::tryPush(const Cmd& cmd) {
    std::scoped_lock lock(producer_mutex_);
    if (!accepting_) {
        return CommandSubmitResult::CLOSED;
    }
    return queue_.tryPush(cmd) ? CommandSubmitResult::ACCEPTED : CommandSubmitResult::FULL;
}

bool CommandMailbox::tryPop(Cmd& cmd) {
    return queue_.tryPop(cmd);
}

void CommandMailbox::open() {
    std::scoped_lock lock(producer_mutex_);
    accepting_ = true;
}

void CommandMailbox::close() {
    std::scoped_lock lock(producer_mutex_);
    accepting_ = false;
}

void CommandMailbox::reset() {
    std::scoped_lock lock(producer_mutex_);
    queue_.clear();
    accepting_ = false;
}

bool CommandMailbox::empty() const {
    return queue_.empty();
}
}  // namespace mle::audio
