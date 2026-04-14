/**
 * @file RuntimeConfig.h
 * @brief Runtime configuration system with typed access and event-based listeners.
 *
 * This file defines the RuntimeConfig singleton and RuntimeConfigListener helper class.
 * It provides:
 * - Registration of known runtime keys with human-readable descriptions
 * - Command-line argument parsing (--key value)
 * - Thread-safe runtime value storage
 * - Typed getters (bool, int, uint, float)
 * - Event-style listeners that react to key changes
 *
 * Design notes:
 * - All values are internally stored as strings and parsed on demand.
 * - Listener callbacks are invoked synchronously during RuntimeConfig::set().
 * - Returning true from a listener callback automatically unregisters it.
 * - RuntimeConfigListener uses RAII to guarantee unregistration on destruction.
 *
 * Thread safety:
 * - Key storage is protected by an internal mutex.
 * - Listener registration and dispatch is protected by a separate mutex.
 * - Callbacks should avoid long work and must not call APIs that could cause lock re-entry.
 *
 * Intended usage:
 * - Register keys early during engine initialization via addKey()
 * - Call parseArgs() at startup
 * - Use set() for runtime toggles and debugging features
 * - Attach RuntimeConfigListener for reactive behavior (hot reload flags, debug commands)
 */

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "mle/math/Types.h"
#include "mle/utils/Utils.h"

namespace mle {
class RuntimeConfigListener;
using RuntimeConfigListenerHnd = std::unique_ptr<RuntimeConfigListener>;  ///< Owning handle to a listener instance.
using RuntimeConfigListenerRef = RuntimeConfigListener*;                  ///< Non-owning pointer to a listener instance.

/**
 * @brief Callback signature for runtime config listeners.
 *
 * @param value The value being set for the key (may be empty, depending on call site).
 * @return true to auto-unregister this listener after invocation, false to keep listening.
 */
using RuntimeConfigListenerCallbackFunction = std::function<bool(const std::string& value)>;
using RTCLCbFn = RuntimeConfigListenerCallbackFunction;  ///< Short alias for RuntimeConfigListenerCallbackFunction.

/**
 * @brief A listener object that subscribes to a runtime config key and receives updates when the key is set.
 *
 * The listener is registered via RuntimeConfig::listen() and removed via RuntimeConfig::unlisten().
 * When the associated key is set, the listener callback is invoked with the new value.
 *
 * Callback return value:
 * - true  -> remove this listener from the key (one-shot / self-unregister)
 * - false -> keep listening
 *
 * Lifetime:
 * - RuntimeConfig stores raw pointers to listeners. The caller must ensure the listener outlives
 *   its registration, or use RAII (destructor calls unlisten()).
 *
 * Thread-safety:
 * - Registration/unregistration is protected by RuntimeConfig internal listener mutex.
 * - Callback invocation happens inside RuntimeConfig::set() while holding listener mutex; callbacks
 *   should be quick and must not call back into RuntimeConfig in a way that could deadlock.
 *
 * Typical usage:
 * - Configure with setKey() and setCallback()
 * - Call listen()
 * - Optionally call unlisten()
 */
class RuntimeConfigListener {
  public:
    /// Creates an unconfigured listener (no key, no callback, not listening).
    RuntimeConfigListener() = default;
    /// Unregisters if currently listening.
    ~RuntimeConfigListener();

    MLE_NO_COPY_MOVE(RuntimeConfigListener);

    /**
     * @brief Sets the key this listener observes.
     *
     * If currently listening, this will unlisten() first.
     *
     * @param key RuntimeConfig key string.
     * @return *this for chaining.
     */
    RuntimeConfigListener& setKey(const std::string& key);
    /**
     * @brief Sets the callback invoked when the key is set.
     *
     * If currently listening, this will unlisten() first.
     *
     * @param cb Callback functor. Returning true removes the listener after invocation.
     * @return *this for chaining.
     */
    RuntimeConfigListener& setCallback(RTCLCbFn&& cb);

    /**
     * @brief Registers this listener with RuntimeConfig for its configured key.
     *
     * Requirements:
     * - key_ must be non-empty
     * - cb_ must be non-null
     *
     * Calling listen() when already listening is a no-op.
     */
    void listen();
    /**
     * @brief Unregisters this listener from RuntimeConfig.
     *
     * Calling unlisten() when not listening is a no-op.
     */
    void unlisten();

  private:
    friend class RuntimeConfig;
    std::string key_;         ///< Key this listener is registered for.
    RTCLCbFn cb_{};           ///< Callback invoked on key updates.
    bool listening_ = false;  ///< True if this listener is currently registered.
};

/**
 * @brief Central runtime configuration store with typed getters and event listeners.
 *
 * RuntimeConfig maintains a map of known keys with:
 * - current string value
 * - human-readable description
 *
 * Keys must be registered via addKey() to accept values from parseArgs() and to have descriptions.
 *
 * Thread-safety:
 * - Key map is protected by keys_mutex_.
 * - Listener map is protected by listeners_mutex_.
 *
 * Notes:
 * - Values are stored as strings; typed getters parse on demand.
 * - Unknown command-line keys are rejected (printed to stdout in current implementation).
 * - Listener callbacks are invoked synchronously during set().
 */
class RuntimeConfig {
    MLE_SINGLETON(RuntimeConfig);

  public:
  private:
    /**
     * @brief A registered runtime key with a value and description.
     */
    struct Key {
        std::string value;        ///< Current value (string form). Empty means "unset" for get().
        std::string description;  ///< Description displayed in logs/help.
    };

  public:
    /**
     * @brief Initializes internal built-in debug keys/listeners.
     *
     * Core init calls this during startup.
     *
     * Registers:
     * - rtc.log_all_values
     * - rtc.log_all_descriptions
     * - rtc.log_all
     * - rtc.log (expects a key name as value)
     */
    void init();

    /**
     * @brief Adds a key and its description to the registry.
     *
     * If the key already exists, its value is reset to empty and description is overwritten.
     *
     * @param key Key string.
     * @param desc Human-readable description.
     */
    void addKey(const std::string& key, const std::string& desc);

    /**
     * @brief Parses command line arguments and sets known keys.
     *
     * Supported format:
     * - --my.key          (implicitly sets to "1")
     * - --my.key value    (sets to "value" if next token is not another flag)
     *
     * Only keys previously registered by addKey() are accepted.
     *
     * @param argc Argument count.
     * @param argv Argument vector.
     */
    void parseArgs(int argc, char** argv);

    /**
     * @brief Sets a runtime key value (string form) and notifies listeners.
     *
     * Current behavior:
     * - If value is non-empty: writes into map_ if key exists.
     * - Always: notifies listeners registered for this key (even if key not registered),
     *   passing the given value.
     *
     * @param key Key string.
     * @param value Value string (may be empty).
     */
    void set(const std::string& key, const std::string& value = "");
    /// Sets a key to an integer by converting via std::to_string.
    void set(const std::string& key, int value) { set(key, std::to_string(value)); }
    /// Sets a key to a float by converting via std::to_string.
    void set(const std::string& key, f32 value) { set(key, std::to_string(value)); }
    /// Sets a key to a bool ("1" for true, "0" for false).
    void set(const std::string& key, bool value) { set(key, std::string(value ? "1" : "0")); }

    /// Returns the string value for a key, or std::nullopt if unset or unknown.
    std::optional<std::string> get(const std::string& key) const;
    /// Returns the boolean value for a key, or std::nullopt if unset, unknown, or unparseable.
    std::optional<bool> getBool(const std::string& key) const;
    /// Returns the integer value for a key, or std::nullopt if unset, unknown, or unparseable.
    std::optional<int> getInt(const std::string& key) const;
    /// Returns the unsigned integer value for a key, or std::nullopt if unset, unknown, or unparseable.
    std::optional<u32> getUInt(const std::string& key) const;
    /// Returns the float value for a key, or std::nullopt if unset, unknown, or unparseable.
    std::optional<float> getFloat(const std::string& key) const;

    /**
     * @brief Registers a listener for its configured key.
     *
     * The listener must have a non-empty key_ and a non-null callback.
     * Duplicate registration for the same key is ignored with a warning.
     *
     * @param rtcl Listener pointer. Must remain valid while registered.
     */
    void listen(RuntimeConfigListenerRef rtcl);
    /**
     * @brief Unregisters a listener from its configured key.
     *
     * @param rtcl Listener pointer.
     */
    void unlisten(RuntimeConfigListenerRef rtcl);

    /// Logs the value and description for a specific key.
    void log(const std::string& key) const;
    /// Logs all keys and their current values.
    void logAllValues() const;
    /// Logs all keys and their descriptions.
    void logAllDescriptions() const;
    /// Logs all keys with descriptions and current values.
    void logAll() const {
        logAllDescriptions();
        logAllValues();
    }

  private:
    mutable std::mutex keys_mutex_;             /// Guards map_.
    std::unordered_map<std::string, Key> map_;  /// Registered keys.

    mutable std::mutex listeners_mutex_;                                                    /// Guards key_listeners_.
    std::unordered_map<std::string, std::vector<RuntimeConfigListenerRef>> key_listeners_;  /// Registered listeners.

    RuntimeConfigListener log_all_values_rtcl_;        /// Built-in listener for rtc.log_all_values.
    RuntimeConfigListener log_all_descriptions_rtcl_;  /// Built-in listener for rtc.log_all_descriptions.
    RuntimeConfigListener log_all_rtcl_;               /// Built-in listener for rtc.log_all.
    RuntimeConfigListener log_rtcl_;                   /// Built-in listener for rtc.log.
};

}  // namespace mle
