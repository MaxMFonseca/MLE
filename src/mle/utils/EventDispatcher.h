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
#include <cassert>
#include <memory>
#include <mutex>
#include <tuple>
#include <variant>
#include <vector>

#include "mle/utils/Types.h"
#include "mle/utils/Utils.h"

namespace mle {

namespace ed_policies {
struct SingleThreaded {
    struct Mutex {
        void lock() {}
        void unlock() {}
    };
    using Lock = std::lock_guard<Mutex>;
    using QMutex = Mutex;
    using QLock = Lock;
};

struct ThreadSafe {
    using Mutex = std::recursive_mutex;
    using Lock = std::unique_lock<Mutex>;
    using QMutex = std::mutex;
    using QLock = std::unique_lock<QMutex>;
};
}  // namespace ed_policies

/**
 * @brief A dispatcher that manages listener registration and dispatch for multiple event types.
 *
 * @tparam EventTypes All event types this dispatcher will support.
 */
template <typename Policy, typename... EventTypes>
class EventDispatcher {
  public:
    MLE_NO_COPY_MOVE(EventDispatcher);

    EventDispatcher() = default;
    ~EventDispatcher() = default;

    /**
     * @brief Pointer to a function handling a specific event type.
     * @tparam EventType The event type the callback handles.
     */
    template <typename EventType>
    using CallbackFunction = std::move_only_function<void(const EventType&)>;

    /**
     * @brief RAII object representing a registered listener for a specific event type.
     *
     * The listener automatically unregisters itself on destruction.
     */
    template <typename EventType>
    class Listener final {
      public:
        MLE_NO_COPY_MOVE(Listener);

        ~Listener() { unlisten(); }

        /// Registers the listener with the dispatcher.
        void listen() { dispatcher_.template listen<EventType>(this); }

        /// Unregisters the listener from the dispatcher.
        void unlisten() { dispatcher_.template unlisten<EventType>(this); }

      private:
        using ED = EventDispatcher<Policy, EventTypes...>;
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
        Listener(ED& dispatcher, CallbackFunction<EventType>&& callback) :
            dispatcher_(dispatcher),
            callback_(std::move(callback)) {}

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
        typename Policy::Mutex mutex;                   ///< Mutex for thread-safe listener access.
    };

    /**
     * @brief Creates and registers a new listener for an event type.
     *
     * @tparam EventType The event type.
     * @param callback The function to call on event.
     * @return An owning handle to the listener.
     */
    template <typename EventType>
    ListenerHnd<EventType> makeListener(CallbackFunction<EventType>&& callback) {
        auto& etd = std::get<EventTypeData<EventType>>(event_type_data_);
        auto& listeners = etd.listeners;

        ListenerHnd<EventType> hnd{new Listener<EventType>(*this, std::move(callback))};  // NOLINT new because of private constructor

        typename Policy::Lock lock{etd.mutex};
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
        typename Policy::Lock lock{etd.mutex};
        auto listeners = etd.listeners;

        if (listeners.empty()) {
            if constexpr (MustBeHandled) {
                assert(false && "Event must be handled!");
            }
            return;
        }

        for (auto* listener : listeners) {
            listener->call(event);
        }
    }

    /**
     * @brief Queues an event for deferred dispatch during `poll()`.
     * @tparam EventType The type of the event to queue.
     * @param event The event to queue.
     */
    template <typename EventType>
    void queue(const EventType& event) {
        typename Policy::QLock ql{queue_mtx_};
        queued_events_.emplace_back(event);
    }

    /// Dispatches all queued events in FIFO order.
    void poll() {
        std::vector<std::variant<EventTypes...>> local;
        {
            typename Policy::QLock ql{queue_mtx_};
            local.swap(queued_events_);
        }
        for (auto& event : local) {
            std::visit([this](auto&& ev) { this->dispatch(ev); }, event);
        }
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
    void listen(ListenerRef<EventType> listener) {
        auto& etd = std::get<EventTypeData<EventType>>(event_type_data_);
        typename Policy::Lock lock{etd.mutex};

        if (std::ranges::find(etd.listeners, listener) == etd.listeners.end()) {
            etd.listeners.push_back(listener);
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
    void unlisten(ListenerRef<EventType> listener) {
        auto& etd = std::get<EventTypeData<EventType>>(event_type_data_);
        typename Policy::Lock lock{etd.mutex};
        if (auto it = std::ranges::find(etd.listeners, listener); it != etd.listeners.end()) {
            etd.listeners.erase(it);
        }
    }

    template <typename EventType>
    usize listenersSize() {
        auto& etd = std::get<EventTypeData<EventType>>(event_type_data_);
        typename Policy::Lock lock{etd.mutex};
        return etd.listeners.size();
    }

  private:
    std::tuple<EventTypeData<EventTypes>...> event_type_data_;  ///< Listener data per event type.
    std::vector<std::variant<EventTypes...>> queued_events_;    ///< Events queued for polling.
    typename Policy::QMutex queue_mtx_;                         ///< Mutex for thread-safe queue access.
};

template <typename Variant>
struct EDFromVariant;
template <typename... Ts>
struct EDFromVariant<std::variant<Ts...>> {
    using Type = EventDispatcher<Ts...>;
};

template <typename... EventTypes>
using EventDispatcherST = EventDispatcher<ed_policies::SingleThreaded, EventTypes...>;

template <typename... EventTypes>
using EventDispatcherTS = EventDispatcher<ed_policies::ThreadSafe, EventTypes...>;
}  // namespace mle
