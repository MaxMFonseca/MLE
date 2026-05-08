#include "RuntimeConfig.h"

#include <algorithm>
#include <iostream>
#include <utility>

#include "mle/core/Logger.h"
#include "mle/utils/String.h"

namespace mle {
void RuntimeConfig::init() {
    log_all_values_rtcl_.setKey("rtc.log_all_values")
        .setCallback([this](const std::string&) {
            logAllValues();
            return false;
        })
        .listen();

    log_all_descriptions_rtcl_.setKey("rtc.log_all_descriptions")
        .setCallback([this](const std::string&) {
            logAllDescriptions();
            return false;
        })
        .listen();

    log_all_rtcl_.setKey("rtc.log_all")
        .setCallback([this](const std::string&) {
            logAllDescriptions();
            logAllValues();
            return false;
        })
        .listen();

    log_rtcl_.setKey("rtc.log")
        .setCallback([this](const std::string& value) {
            log(value);
            return false;
        })
        .listen();
}

void RuntimeConfig::addKey(const std::string& key, const std::string& desc) {
    std::scoped_lock<std::mutex> lock(keys_mutex_);
    map_[key] = Key{
        .value = "",
        .description = desc,
    };
}

void RuntimeConfig::parseArgs(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::string arg = argv[i];
        if (arg.starts_with("--")) {
            std::string key = arg.substr(2);
            std::string value = "1";
            if (i + 1 < argc) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                std::string next = argv[i + 1];
                if (!next.starts_with("-")) {
                    value = next;
                    ++i;
                }
            }
            auto it = map_.find(key);
            if (it != map_.end()) {
                it->second.value = value;
            } else {
                std::cout << "unknown command line arg: " << key << '\n';
                continue;
            }
            std::cout << "command line arg: " << key << " = " << value << '\n';
        }
    }
}

void RuntimeConfig::logAllValues() const {
    std::scoped_lock<std::mutex> lock(keys_mutex_);
    MLE_I("RuntimeConfig values:");
    for (const auto& [k, v] : map_) {
        MLE_I("{}. value = {}", k, v.value);
    }
}

void RuntimeConfig::logAllDescriptions() const {
    std::scoped_lock<std::mutex> lock(keys_mutex_);
    MLE_I("RuntimeConfig descriptions:");
    for (const auto& [k, v] : map_) {
        MLE_I("{}. desc = {}", k, v.description);
    }
}

void RuntimeConfig::log(const std::string& key) const {
    std::scoped_lock<std::mutex> lock(keys_mutex_);
    auto it = map_.find(key);
    if (it != map_.end()) {
        MLE_I("RuntimeConfig: {} = {}, desc = {}", key, it->second.value, it->second.description);
    } else {
        MLE_I("RuntimeConfig: key {} not found.", key);
    }
}

/// FIXME: do not lock everything! This could be done with atomics if I give up on string storage...
/// or maybe simply split the locks for keys and listeners
void RuntimeConfig::set(const std::string& key, const std::string& value) {
    if (!value.empty()) {
        std::scoped_lock<std::mutex> lock(keys_mutex_);
        MLE_I("RuntimeConfig set: {} = {}", key, value);
        if (auto kit = map_.find(key); kit != map_.end()) {
            kit->second.value = value;
        }
    }

    std::vector<RuntimeConfigListenerRef> listeners;
    {
        std::scoped_lock<std::mutex> lock(listeners_mutex_);
        if (auto it = key_listeners_.find(key); it != key_listeners_.end()) {
            listeners.assign(it->second.begin(), it->second.end());
        }
    }

    std::vector<RuntimeConfigListenerRef> to_remove;
    for (auto* l : listeners) {
        if (l && l->cb_ && l->cb_(value)) {
            to_remove.push_back(l);
        }
    }

    if (!to_remove.empty()) {
        std::scoped_lock<std::mutex> lock(listeners_mutex_);
        if (auto it = key_listeners_.find(key); it != key_listeners_.end()) {
            for (auto* dead : to_remove) {
                auto& vec = it->second;
                std::ignore = std::ranges::remove(vec, dead);
            }
        }
    }
}

std::optional<std::string> RuntimeConfig::get(const std::string& key) const {
    std::scoped_lock<std::mutex> lock(keys_mutex_);
    auto it = map_.find(key);
    if (it != map_.end() && !it->second.value.empty()) {
        return it->second.value;
    }
    return std::nullopt;
}

std::optional<bool> RuntimeConfig::getBool(const std::string& key) const {
    auto vr = get(key);
    if (vr) {
        std::string v = toLower(vr.value());
        if (v == "1" || v == "true" || v == "y" || v == "yes" || v == "on") {
            return true;
        }
        if (v == "0" || v == "false" || v == "n" || v == "no" || v == "off") {
            return false;
        }
    }
    return std::nullopt;
}

std::optional<int> RuntimeConfig::getInt(const std::string& key) const {
    auto val = get(key);
    if (val) {
        auto v = strTo<int>(val.value());
        if (v) {
            return v.value();
        }
    }
    return std::nullopt;
}

std::optional<u32> RuntimeConfig::getUInt(const std::string& key) const {
    auto val = get(key);
    if (val) {
        auto v = strTo<u32>(val.value());
        if (v) {
            return v.value();
        }
    }
    return std::nullopt;
}

std::optional<float> RuntimeConfig::getFloat(const std::string& key) const {
    auto val = get(key);
    if (val) {
        auto v = strTo<f32>(val.value());
        if (v) {
            return v.value();
        }
    }
    return std::nullopt;
}

void RuntimeConfig::listen(RuntimeConfigListenerRef rtcl) {
    if (rtcl->key_.empty()) {
        MLE_W("RTCL {} has empty key, cannot listen.", (void*)rtcl);
        return;
    }
    std::scoped_lock<std::mutex> lock(listeners_mutex_);
    auto& kls = key_listeners_[rtcl->key_];
    if (std::ranges::find(kls, rtcl) == kls.end()) {
        kls.push_back(rtcl);
    } else {
        MLE_W("RTCL {} already registered for key: {}", (void*)rtcl, rtcl->key_);
    }
}

void RuntimeConfig::unlisten(RuntimeConfigListenerRef l) {
    std::scoped_lock<std::mutex> lock(listeners_mutex_);
    auto it = key_listeners_.find(l->key_);
    if (it != key_listeners_.end()) {
        auto& vec = it->second;
        auto [beg, end] = std::ranges::remove(vec, l);
        vec.erase(beg, end);
        if (vec.empty()) {
            key_listeners_.erase(it);
        }
    }
}

RuntimeConfigListener::~RuntimeConfigListener() {
    unlisten();
}

RuntimeConfigListener& RuntimeConfigListener::setKey(const std::string& key) {
    unlisten();
    key_ = key;
    return *this;
};

RuntimeConfigListener& RuntimeConfigListener::setCallback(RTCLCbFn&& cb) {
    unlisten();
    cb_ = std::move(cb);
    return *this;
};

void RuntimeConfigListener::unlisten() {
    if (!listening_) {
        return;
    }
    RuntimeConfig::i().unlisten(this);
    listening_ = false;
};

void RuntimeConfigListener::listen() {
    if (listening_) {
        return;
    }
    if (key_.empty() || cb_ == nullptr) {
        MLE_E("Cannot listen RTCL {} with empty key({}) or null callback({}).", (void*)this, key_.empty(), cb_ == nullptr);
        return;
    }
    RuntimeConfig::i().listen(this);
    listening_ = true;
};
}  // namespace mle
