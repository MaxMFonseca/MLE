#include "mle/client/layers/TerminalLayer.h"

#include "mle/core/Logger.h"
#include "mle/core/PerfTracker.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/Entt.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/renderable/Text.h"
#include "mle/utils/Flags.h"
#include "mle/utils/String.h"
#include "mle/window/TextBox.h"
#include "mle/window/Types.h"

namespace mle::client {
void TerminalLayer::init() {
    toggle_key_listener_ = std::make_unique<KeyListener>([this]() { toggle(); }, Key::M, KeyState::PRESSED, KeyModFlagBits::CTRL);
    enter_key_listener_ = std::make_unique<KeyListener>([this]() { onEnter(); }, Key::ENTER, KeyState::PRESSED);
    up_key_listener_ = std::make_unique<KeyListener>([this]() { commandHistoryUp(); }, Key::UP, KeyState::PRESSED);
    down_key_listener_ = std::make_unique<KeyListener>([this]() { commandHistoryDown(); }, Key::DOWN, KeyState::PRESSED);
    clear_key_listener_ = std::make_unique<KeyListener>([this]() { clearCmd(); }, Key::ESCAPE, KeyState::PRESSED);
    toggle_key_listener_->listen();

    ui_.setRoot("mle/ui/terminal_layer");

    auto terminal_text_entt_r = ui_.getE(std::array<const std::string_view, 1>{"text"});
    if (terminal_text_entt_r.has_value()) {
        terminal_text_entt_ = terminal_text_entt_r->e();
    } else {
        MLE_E("TerminalLayer: could not find terminal text entity");
    }
}

void TerminalLayer::commandHistoryUp() {
    if (!enabled_) {
        return;
    }
    if (current_command_idx_ == as<int>(command_history_.size()) - 1 || command_history_.empty()) {
        return;
    }
    ui::Entt e{ui_, terminal_text_entt_};
    auto text_r = mle::ui::renderable::Text::getFromEntt(e);
    if (!text_r.has_value()) {
        MLE_W("TerminalLayer: could not get Text component from terminal text entity. error:{}", text_r.error());
        return;
    }
    auto& text_comp = text_r.value().get();
    if (current_command_idx_ == -1) {
        auto t = text_comp.input_tb->getText();
        if (!t.empty()) {
            command_history_.push_front(t);
            current_command_idx_ = (current_command_idx_ + 1) % COMMAND_HISTORY_MAX_SIZE;
        }
    }

    current_command_idx_ = (current_command_idx_ + 1) % COMMAND_HISTORY_MAX_SIZE;
    std::u32string history_text = command_history_.at(current_command_idx_);
    text_comp.input_tb->setText(history_text);
    e.addFlag<ui::comp::RequestExternalBoundsUpdateFlag>();
}

void TerminalLayer::commandHistoryDown() {
    if (!enabled_) {
        return;
    }
    if (current_command_idx_ <= 0) {
        clearCmd();
        current_command_idx_ = -1;
        return;
    }

    ui::Entt e{ui_, terminal_text_entt_};
    auto text_r = mle::ui::renderable::Text::getFromEntt(e);
    if (!text_r.has_value()) {
        MLE_W("TerminalLayer: could not get Text component from terminal text entity. error:{}", text_r.error());
        return;
    }

    auto& text_comp = text_r.value().get();
    current_command_idx_ = (current_command_idx_ - 1) % COMMAND_HISTORY_MAX_SIZE;
    std::u32string history_text = command_history_.at(current_command_idx_);
    text_comp.input_tb->setText(history_text);
    e.addFlag<ui::comp::RequestExternalBoundsUpdateFlag>();
}

void TerminalLayer::clearCmd() {
    if (!enabled_) {
        return;
    }
    ui::Entt e{ui_, terminal_text_entt_};
    e.apply("text_input_clear");
}

void TerminalLayer::toggle() {
    if (terminal_text_entt_ == entt::null) {
        MLE_W("TerminalLayer: terminal text entity is null, cannot toggle terminal layer");
        return;
    }

    enabled_ = !enabled_;
    MLE_D("Terminal layer enabled: {}", enabled_);

    if (enabled_) {
        enableInput();
    } else {
        disableInput();
    }
}

void TerminalLayer::enableInput() {
    ui::Entt e{ui_, terminal_text_entt_};
    e.apply("text_input_enable");
    current_command_idx_ = 0;

    enter_key_listener_->listen();
    up_key_listener_->listen();
    down_key_listener_->listen();
    clear_key_listener_->listen();
}

void TerminalLayer::disableInput() {
    ui::Entt e{ui_, terminal_text_entt_};
    e.apply("text_input_clear");
    e.apply("text_input_disable");

    enter_key_listener_->unlisten();
    up_key_listener_->unlisten();
    down_key_listener_->unlisten();
    clear_key_listener_->unlisten();
}

void TerminalLayer::onEnter() {
    if (!enabled_) {
        return;
    }
    ui::Entt e{ui_, terminal_text_entt_};

    auto text_r = mle::ui::renderable::Text::getFromEntt(e);
    if (!text_r.has_value()) {
        MLE_W("TerminalLayer: could not get Text component from terminal text entity. error:{}", text_r.error());
        return;
    }

    auto& text_comp = text_r.value().get();

    std::u32string input_text;
    if (text_comp.input_tb) {
        input_text = text_comp.input_tb->getText();
    }
    if (input_text.empty()) {
        return;
    }

    if (command_history_.size() > COMMAND_HISTORY_MAX_SIZE) {
        command_history_.pop_back();
    }
    command_history_.push_front(input_text);
    current_command_idx_ = -1;

    for (const auto& i : command_history_) {
        MLE_C("{}", toUtf8(i));
    }

    std::string input_text_utf8 = toUtf8(input_text);
    input_text_utf8 = trim(input_text_utf8);

    MLE_I("Terminal input: {}", input_text_utf8);

    if (!input_text_utf8.empty()) {
        const auto space = input_text_utf8.find_first_of(' ');
        const std::string key = (space == std::string_view::npos) ? input_text_utf8 : input_text_utf8.substr(0, space);
        const std::string args = (space == std::string_view::npos) ? "" : input_text_utf8.substr(space + 1);

        RuntimeConfig::i().set(key, args);
    }

    clearCmd();
}

void TerminalLayer::shutdown() {
    ui_.clear();
};

void TerminalLayer::update() {
    if (!enabled_) {
        return;
    }
    ui_.update();
}

ImageRef TerminalLayer::render([[maybe_unused]] f64 dt) {
    if (!enabled_) {
        return nullptr;
    }
    return ui_.render();
}

}  // namespace mle::client
