#include "PerfLayer.h"

#include "mle/client/Client.h"
#include "mle/core/PerfTracker.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/Entt.h"
#include "mle/ui/renderable/Text.h"

namespace mle::client {
void PerfLayer::init() {
    perf_listener_ = std::make_unique<PerfNewSamplesListener>([this](const PerfTracker::Samples& samples) {
        std::scoped_lock lock{new_samples_mutex_};
        new_samples_ = samples;
    });
    toggle_key_listener_ = std::make_unique<KeyListener>([this]() { toggle(); }, Key::F3, KeyState::PRESSED);
    toggle_key_listener_->listen();

    ui_.setRoot("mle/ui/perf_layer");
}

void PerfLayer::toggle() {
    enabled_ = !enabled_;
    MLE_I("Perf layer enabled: {}", enabled_);
}

void PerfLayer::shutdown() {
    ui_.clear();
};

void PerfLayer::update() {
    if (!enabled_) {
        return;
    }
    if (!new_samples_.empty()) {
        auto parsed = parseNewSamples();
        std::string text;
        for (auto& c : parsed) {
            text += c.first + ":";
            if (c.second.contains("!")) {
                auto& sc = c.second["!"];
                text += fmt::format(" avg: {:.2f}us calls: {} ", sc.per_call_us, sc.calls_last_sec);
                c.second.erase("!");
            }
            text += "\n";
            for (auto& sc : c.second) {
                text += fmt::format("  {}: avg: {:.2f}us calls: {}\n", sc.first, sc.second.per_call_us, sc.second.calls_last_sec);
            }
        }

        {
            auto e_r = ui_.getE(std::array<const std::string_view, 2>{"left", "text"});
            if (e_r.has_value()) {
                auto& e = e_r.value();
                e.apply("text", Client::i().lua().createObject(text));
            } else {
                MLE_W("PerfLayer: could not find entity to update performance info");
            }
        }

        {
            auto e_r = ui_.getE(std::array<const std::string_view, 2>{"right", "text"});
            if (e_r.has_value()) {
                auto& e = e_r.value();
                e.apply("text", Client::i().lua().createObject(Renderer::i().vk().makePerfString()));
            } else {
                MLE_W("PerfLayer: could not find entity to update GPU performance info");
            }
        }
    };
    ui_.update();
}

PerfLayer::ParsedSamples PerfLayer::parseNewSamples() {
    ParsedSamples ret;

    for (const auto& s : new_samples_) {
        std::string_view full{s.name};
        const auto dot = full.find_first_of('.');

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
    if (!enabled_) {
        return nullptr;
    }
    return ui_.render();
}

void PerfLayer::renderTo(ImageRef target) {
    if (!enabled_) {
        return;
    }
    auto* ui_img = ui_.render();
    if (ui_img != nullptr) {
        target->blend(Renderer::i().frameRenderer().cmd(), *ui_img, 1, {}, {0, 0, as<int>(ui_img->getExtent().x), as<int>(ui_img->getExtent().y)});
    }
}
}  // namespace mle::client
