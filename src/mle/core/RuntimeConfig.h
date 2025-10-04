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

class RuntimeConfig {
    MLE_SINGLETON(RuntimeConfig);

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
    float getFloat(const std::string& key, f32 def = 0.0F) const;

    RuntimeConfigListenerHnd listen(const std::string& key, std::move_only_function<void(const std::string& value)> cb);

  private:
    friend class RuntimeConfigListener;
    void removeListener(RuntimeConfigListenerRef l);

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> map_;
    std::unordered_map<std::string, std::vector<RuntimeConfigListenerRef>> key_listeners_;
};

class RuntimeConfigListener {
  public:
    ~RuntimeConfigListener();

    MLE_NO_COPY_MOVE(RuntimeConfigListener);

  private:
    friend class RuntimeConfig;
    RuntimeConfigListener(std::string key, std::move_only_function<void(const std::string& value)> cb);
    const std::string key_;
    std::move_only_function<void(const std::string&)> cb_;
};
}  // namespace mle
