/**
 * @file
 * @brief Template-based event dispatcher for strongly-typed event systems.
 *
 * This header defines a generic `EventDispatcher<EventTypes...>` class that supports:
 * - Static compile-time event types
 * - Type-safe listener registration and dispatch
 * - Event queuing and polling
 *
 * Listeners are created with RAII handles using `makeEventListener`, and are automatically
 * unregistered on destruction. The dispatcher supports multithreading-safe registration and dispatch,
 * though multithreaded dispatch is currently unimplemented.
 *
 * It is recomended that you typedef the dispatcher like this:
 * @code
 * using ED = EventDispatcher<events::WindowClose, events::WindowResize, events::WindowIconify>;
 * @endcode
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <expected>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <variant>
#include <vector>

#include "Types.h"

namespace mle {

/**
 * @brief A dispatcher that manages listener registration and dispatch for multiple event types.
 *
 * @tparam EventTypes All event types this dispatcher will support.
 */
template <typename... EventTypes>
class EventDispatcher {
  public:
    using ED = EventDispatcher<EventTypes...>;  ///< Dispatcher type alias.
    using EDRef = ED*;                          ///< Non-owning dispatcher handle.

    /**
     * @brief Pointer to a function handling a specific event type.
     * @tparam EventType The event type the callback handles.
     */
    template <typename EventType>
    using CallbackFunction = std::function<void(const EventType&)>;

    /**
     * @brief RAII object representing a registered listener for a specific event type.
     *
     * The listener automatically unregisters itself on destruction.
     */
    template <typename EventType>
    class Listener final {
      public:
        Listener(const Listener&) = delete;
        Listener& operator=(const Listener&) = delete;
        Listener(Listener&&) = delete;
        Listener& operator=(Listener&&) = delete;

        ~Listener() { unsign(); }

      private:
        friend ED;
        ED& dispatcher_;                        ///< Dispatcher that owns this listener.
        CallbackFunction<EventType> callback_;  ///< Function to call when the event is dispatched.

        /**
         * @brief Constructs a new event listener (private).
         *
         * This constructor is private and should not be called directly.
         * Use `makeEventListener()` from the dispatcher to create and register listeners properly.
         *
         * @param dispatcher Reference to the owning dispatcher.
         * @param callback Function pointer to be invoked on event dispatch.
         *
         * @see EventDispatcher::makeEventListener
         */
        Listener(ED& dispatcher, CallbackFunction<EventType> callback) :
            dispatcher_(dispatcher),
            callback_(callback) {}

        /// Registers the listener with the dispatcher.
        void sign() { dispatcher_.signEventListener<EventType>(this); }

        /// Unregisters the listener from the dispatcher.
        void unsign() { dispatcher_.unsignEventListener<EventType>(this); }

        /// Invokes the callback for the event.
        void call(const EventType& event) { callback_(event); }
    };

    /// Owning handle to a listener.
    template <typename EventType>
    using ListenerHnd = std::unique_ptr<Listener<EventType>>;

    /// Non-owning reference to a listener.
    template <typename EventType>
    using ListenerRef = Listener<EventType>*;

    /**
     * @brief Storage for all listeners and mutex per event type.
     * @tparam EventType The event type this data is associated with.
     */
    template <typename EventType>
    struct EventTypeData {
        std::vector<ListenerRef<EventType>> listeners;  ///< List of registered listeners.
        // std::mutex mutex;                               ///< Mutex for synchronizing access to listeners.
    };

    /**
     * @brief Creates and registers a new listener for an event type.
     *
     * @tparam EventType The event type.
     * @param callback The function to call on event.
     * @return An owning handle to the listener.
     */
    template <typename EventType>
    ListenerHnd<EventType> makeEventListener(CallbackFunction<EventType> callback) {
        auto& etd = std::get<EventTypeData<EventType>>(event_type_data_);
        auto& listeners = etd.listeners;
        // std::lock_guard<std::mutex> lock(etd.mutex);

        ListenerHnd<EventType> hnd{new Listener<EventType>(*this, callback)};  // NOLINT new because of private constructor
        listeners.push_back(hnd.get());
        return hnd;
    }

    /**
     * @brief Dispatches an event immediately to all registered listeners.
     *
     * @tparam EventType The type of the event to dispatch.
     * @tparam MustBeHandled If true, asserts if no listener is registered.
     * @param event The event to dispatch.
     */
    template <typename EventType, bool MustBeHandled = false>
    void dispatch(const EventType& event) {
        auto& etd = std::get<EventTypeData<EventType>>(event_type_data_);
        auto& listeners = etd.listeners;

        if (listeners.empty()) {
            if constexpr (MustBeHandled) {
                assert(false && "Event must be handled!");
            }
            return;
        }

        // std::lock_guard<std::mutex> lock(etd.mutex);

        for (auto listener : listeners) {
            listener->call(event);
        }
    }

    // TODO: this
    //
    // template <typename EventType>
    // void dispatchMT(const EventType& event, bool wait_for_all) {
    //     auto& etd = std::get<EventTypeData<EventType>>(event_type_data_);
    //     auto& handlers = etd.handlers;
    //
    //     if (handlers.empty()) {
    //         return;
    //     }
    //
    //     std::lock_guard<std::mutex> lock(etd.mutex);
    //
    //     std::vector<std::future<void>> futures;
    //     futures.reserve(handlers.size());
    //
    //     for (auto& [user, callback] : handlers) {
    //         futures.emplace_back(std::async(std::launch::async, callback, user, event));
    //     }
    //
    //     if (wait_for_all) {
    //         for (auto& future : futures) {
    //             future.wait();
    //         }
    //     }
    // }

    /**
     * @brief Queues an event for deferred dispatch during `poll()`.
     * @tparam EventType The type of the event to queue.
     * @param event The event to queue.
     */
    template <typename EventType>
    void queue(const EventType& event) {
        queued_events_.emplace_back(event);
    }

    /// Dispatches all queued events in FIFO order.
    void poll() {
        for (auto& event : queued_events_) {
            std::visit([this](auto&& event) { dispatch(event); }, event);
        }
        queued_events_.clear();
    }

  private:
    /**
     * @brief Registers a listener. (private)
     *
     * Cant be called directly, use `EventListener::sign()` instead.
     *
     * @tparam EventType The event type.
     * @param listener The listener to register.
     *
     * @see EventListener::sign
     */
    template <typename EventType>
    void signEventListener(ListenerRef<EventType> listener) {
        auto& etd = std::get<EventTypeData<EventType>>(event_type_data_);
        auto& listeners = etd.listeners;
        std::lock_guard<std::mutex> lock(etd.mutex);
        if (std::ranges::find(listeners, listener) == listeners.end()) {
            listeners.push_back(listener);
        }
    }

    /**
     * @brief Unregisters a listener. (private)
     *
     * Cant be called directly, use `EventListener::unsign()` instead.
     *
     * @tparam EventType The event type.
     * @param listener The listener to remove.
     *
     * @see EventListener::unsign
     */
    template <typename EventType>
    void unsignEventListener(ListenerRef<EventType> listener) {
        auto& etd = std::get<EventTypeData<EventType>>(event_type_data_);
        auto& listeners = etd.listeners;
        // std::lock_guard<std::mutex> lock(etd.mutex);
        if (auto it = std::ranges::find(listeners, listener); it != listeners.end()) {
            listeners.erase(it);
        }
    }

    template <typename EventType>
    usize listenersSize() {
        return std::get<EventTypeData<EventType>>(event_type_data_).listeners.size();
    }

  private:
    std::tuple<EventTypeData<EventTypes>...> event_type_data_;  ///< Listener data per event type.
    std::vector<std::variant<EventTypes...>> queued_events_;    ///< Events queued for polling.
};

template <typename Variant>
struct EDFromVariant;
template <typename... Ts>
struct EDFromVariant<std::variant<Ts...>> {
    using Type = EventDispatcher<Ts...>;
};
}  // namespace mle
