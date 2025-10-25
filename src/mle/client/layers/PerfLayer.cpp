#include "PerfLayer.h"

#include "mle/core/PerfTracker.h"

namespace mle::client {
void PerfLayer::init() {
    perf_listener_ = std::make_unique<PerfNewSamplesListener>([this](const PerfTracker::Samples& samples) {
        std::scoped_lock lock{new_samples_mutex_};
        new_samples_ = samples;
    });
    toggle_key_listener_ = std::make_unique<KeyListener>([this]() { toggle(); }, Key::A, KeyState::PRESSED);
    toggle_key_listener_2_ = std::make_unique<KeyListener>([this]() { toggle(); }, Key::F3, KeyState::PRESSED);
}

void PerfLayer::toggle() {
    enabled_ = !enabled_;
    MLE_I("Perf layer enabled: {}", enabled_);
}

void PerfLayer::shutdown() {};

void PerfLayer::update() {
    if (!new_samples_.empty() && enabled_) {
        auto parsed = parseNewSamples();
        for (auto& c : parsed) {
            for (auto& sc : c.second) {
                MLE_C("c: {}, sc: {}, last_sec_ns: {}, per_call_us: {}, calls: {}", c.first, sc.first, sc.second.last_sec_ns, sc.second.per_call_us,
                      sc.second.calls_last_sec);
            }
        }
    };
}

PerfLayer::ParsedSamples PerfLayer::parseNewSamples() {
    ParsedSamples ret;

    for (const auto& s : new_samples_) {
        std::string_view full{s.name};
        const auto dot = full.find('.');

        const std::string_view category_sv = (dot == std::string_view::npos) ? full : full.substr(0, dot);

        const std::string_view subcategory_sv = (dot == std::string_view::npos || dot + 1 >= full.size()) ? std::string_view{"!"} : full.substr(dot + 1);

        auto& cat_map = ret[std::string(category_sv)];
        auto& out = cat_map[std::string(subcategory_sv)];
        out.last_sec_ns = s.last_sec_ns;
        out.per_call_us = s.per_call_us;
        out.calls_last_sec = s.calls_last_sec;
    }

    new_samples_.clear();
    return ret;
}

ImageRef PerfLayer::render([[maybe_unused]] f64 dt) {
    return nullptr;
}

}  // namespace mle::client
