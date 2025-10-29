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
using RuntimeConfigListenerHnd = std::unique_ptr<RuntimeConfigListener>;
using RuntimeConfigListenerRef = RuntimeConfigListener*;

using RuntimeConfigListenerCallbackFunction = std::function<bool(const std::string& value)>;
using RTCLCbFn = RuntimeConfigListenerCallbackFunction;

class RuntimeConfigListener {
  public:
    RuntimeConfigListener() = default;
    ~RuntimeConfigListener();

    MLE_NO_COPY_MOVE(RuntimeConfigListener);

    RuntimeConfigListener& setKey(const std::string& key);
    RuntimeConfigListener& setCallback(RTCLCbFn&& cb);

    void listen();
    void unlisten();

  private:
    friend class RuntimeConfig;
    std::string key_;
    RTCLCbFn cb_{};
    bool listening_ = false;
};

class RuntimeConfig {
    MLE_SINGLETON(RuntimeConfig);

  public:
  private:
    struct Key {
        std::string value;
        std::string description;
    };

  public:
    void init();

    void addKey(const std::string& key, const std::string& desc);

    void parseArgs(int argc, char** argv);

    void set(const std::string& key, const std::string& value = "");
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
    void logAll() const {
        logAllDescriptions();
        logAllValues();
    }

  private:
    mutable std::mutex keys_mutex_;
    std::unordered_map<std::string, Key> map_;

    mutable std::mutex listeners_mutex_;
    std::unordered_map<std::string, std::vector<RuntimeConfigListenerRef>> key_listeners_;

    RuntimeConfigListener log_all_values_rtcl_;
    RuntimeConfigListener log_all_descriptions_rtcl_;
    RuntimeConfigListener log_all_rtcl_;
    RuntimeConfigListener log_rtcl_;
};

}  // namespace mle
