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
namespace mle {
class RuntimeConfigListener;
using RuntimeConfigListenerHnd = std::unique_ptr<RuntimeConfigListener>;
using RuntimeConfigListenerRef = RuntimeConfigListener*;

class RuntimeConfig {
    MLE_SINGLETON(RuntimeConfig);

    using ListenerCbFn = std::move_only_function<bool(const std::string& value)>;

  public:
    void parseArgs(int argc, char** argv);

    void logAll() const;
    void log(const std::string& key) const;

    void set(const std::string& key, const std::string& value);
    void set(const std::string& key, int value) { set(key, std::to_string(value)); }
    void set(const std::string& key, f32 value) { set(key, std::to_string(value)); }
    void set(const std::string& key, bool value) { set(key, std::string(value ? "1" : "0")); }

    std::string getString(const std::string& key, const std::string& def = "") const;
    bool getBool(const std::string& key, bool def = false) const;
    int getInt(const std::string& key, int def = 0) const;
    u32 getUInt(const std::string& key, u32 def = 0) const;
    float getFloat(const std::string& key, f32 def = 0.0F) const;

    RuntimeConfigListenerHnd listen(const std::string& key, ListenerCbFn cb);

  private:
    friend class RuntimeConfigListener;
    void removeListener(RuntimeConfigListenerRef l);

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> map_;
    std::unordered_map<std::string, std::vector<RuntimeConfigListenerRef>> key_listeners_;
};

class RuntimeConfigListener {
  public:
    using CbFn = RuntimeConfig::ListenerCbFn;

  public:
    ~RuntimeConfigListener();

    MLE_NO_COPY_MOVE(RuntimeConfigListener);

  private:
    friend class RuntimeConfig;
    RuntimeConfigListener(std::string key, CbFn cb);
    const std::string key_;
    CbFn cb_;
};
}  // namespace mle
