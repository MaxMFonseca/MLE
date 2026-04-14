#include "Events.h"

#include "mle/core/Assert.h"

namespace mle::ui::system {
Events::Events(UI& ui) :
    ui_(ui) {
    MLE_I("Creating Events system");
}

EventListener& EventListener::setTargetEvent(const std::string& event_name) {
    unlisten();
    event_name_ = event_name;
    return *this;
};

EventListener& EventListener::setCallback(EventCallbackFunction&& cb) {
    unlisten();
    cb_ = std::move(cb);
    return *this;
};

void EventListener::listen() {
    if (listening_) {
        return;
    }
    if (event_name_.empty() || cb_ == nullptr) {
        MLE_E("Cannot listen EventListener {} with empty event_name({}) or null callback({}).", (void*)this, event_name_.empty(), cb_ == nullptr);
        return;
    }
    events_.listen(this);
    listening_ = true;
};

void EventListener::unlisten() {
    if (!listening_) {
        return;
    }
    events_.unlisten(this);
    listening_ = false;
};

void Events::listen(EventListener* listener) {
    MLE_ASSERT_LOG(listener != nullptr, "Cannot listen null EventListener");
    MLE_ASSERT_LOG(!listener->event_name_.empty(), "Cannot listen EventListener with empty event_name");
    MLE_ASSERT_LOG(listener->cb_ != nullptr, "Cannot listen EventListener with null callback");

    auto& vec = listeners_[listener->event_name_];

    if (std::ranges::find(vec, listener) == vec.end()) {
        vec.push_back(listener);
    } else {
        MLE_W("EventListener {} already registered for event: {}", (void*)listener, listener->event_name_);
    }
}

void Events::unlisten(EventListener* listener) {
    MLE_ASSERT_LOG(listener != nullptr, "Cannot unlisten null EventListener");

    auto it = listeners_.find(listener->event_name_);
    if (it != listeners_.end()) {
        auto& vec = it->second;
        auto [beg, end] = std::ranges::remove(vec, listener);
        vec.erase(beg, end);
        if (vec.empty()) {
            listeners_.erase(it);
        }
    }
}

void Events::dispatchEvents() {
    for (const auto& [event_name, obj] : event_queue_) {
        auto it = listeners_.find(event_name);
        if (it != listeners_.end()) {
            for (auto* listener : it->second) {
                listener->cb_(obj);
            }
        }
    }
    event_queue_.clear();
};
}  // namespace mle::ui::system
