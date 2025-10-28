#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "mle/math/Types.h"
#include "mle/utils/Utils.h"

// TODO: This could use some love, I should probably refactor it
// I should use 1 mutex per key
// I need a hint system

// 1 mutex per key
// valid keys must be declared
// persistent config should be marked as such
// listeners can have hints, and create a 'temporary' entry if the key doesn't exist yet

namespace mle {
class RuntimeConfigListener;
using RuntimeConfigListenerHnd = std::unique_ptr<RuntimeConfigListener>;
using RuntimeConfigListenerRef = RuntimeConfigListener*;

class RuntimeConfig {
    MLE_SINGLETON(RuntimeConfig);

  public:
    using ListenerCbFn = std::move_only_function<bool(const std::string& value)>;

  private:
    struct Key {
        std::string value;
        std::string description;
    };

  public:
    void addKey(const std::string& key, const std::string& desc);

    void parseArgs(int argc, char** argv);

    void set(const std::string& key, const std::string& value);
    void set(const std::string& key, int value) { set(key, std::to_string(value)); }
    void set(const std::string& key, f32 value) { set(key, std::to_string(value)); }
    void set(const std::string& key, bool value) { set(key, std::string(value ? "1" : "0")); }

    std::optional<std::string> get(const std::string& key) const;
    std::optional<bool> getBool(const std::string& key) const;
    std::optional<int> getInt(const std::string& key) const;
    std::optional<u32> getUInt(const std::string& key) const;
    std::optional<float> getFloat(const std::string& key) const;

    void listen(RuntimeConfigListenerRef rtcl);
    void unlisten(RuntimeConfigListenerRef rtcl);

    void log(const std::string& key) const;
    void logAllValues() const;
    void logAllDescriptions() const;

  private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Key> map_;
    std::unordered_map<std::string, std::vector<RuntimeConfigListenerRef>> key_listeners_;
};

class RuntimeConfigListener {
  public:
    using CbFn = RuntimeConfig::ListenerCbFn;

  public:
    RuntimeConfigListener() = default;
    ~RuntimeConfigListener();

    MLE_NO_COPY_MOVE(RuntimeConfigListener);

    RuntimeConfigListener& setKey(const std::string& key);
    RuntimeConfigListener& setCallback(CbFn&& cb);

    void listen();
    void unlisten();

  private:
    friend class RuntimeConfig;
    std::string key_;
    CbFn cb_;
    bool listening_ = false;
};
}  // namespace mle
