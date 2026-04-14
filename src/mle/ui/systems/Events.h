#pragma once

#include "../Types.h"

namespace mle::ui::system {

using EventCallbackFunction = std::function<void(const sol::object& obj)>;

class EventListener {
  public:
    explicit EventListener(Events& events) :
        events_(events) {}
    ~EventListener() { unlisten(); }

    MLE_NO_COPY_MOVE(EventListener);

    EventListener& setTargetEvent(const std::string& event_name);
    EventListener& setCallback(EventCallbackFunction&& cb);

    void listen();
    void unlisten();

  public:
    Events& events_;

    std::string event_name_;
    EventCallbackFunction cb_{};
    bool listening_ = false;
};

class Events {
  public:
    MLE_NO_COPY_MOVE(Events);

    explicit Events(UI& ui);
    ~Events() = default;

    void enqueueEvent(const std::string& event_name, const sol::object& obj = {}) { event_queue_.emplace_back(event_name, obj); }
    void dispatchEvents();

    void listen(EventListener* listener);
    void unlisten(EventListener* listener);

  private:
    [[maybe_unused]] UI& ui_;

    std::vector<std::pair<std::string, sol::object>> event_queue_;
    std::map<std::string, std::vector<EventListener*>> listeners_;
};
}  // namespace mle::ui::system
