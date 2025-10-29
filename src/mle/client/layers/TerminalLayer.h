#pragma once

#include <string>

#include "mle/client/Layer.h"
#include "mle/core/PerfTracker.h"
#include "mle/ui/UI.h"
#include "mle/utils/ECS.h"
#include "mle/window/UserInputManager.h"

namespace mle::client {
class TerminalLayer : public Layer {
  public:
    MLE_NO_COPY_MOVE(TerminalLayer)

    TerminalLayer() = default;
    ~TerminalLayer() override = default;

    void init() override;
    void shutdown() override;

    void update() override;
    ImageRef render(f64 dt) override;
    void renderTo(ImageRef target) override;

  private:
    void toggle();
    void onEnter();

    void enableInput();
    void disableInput();

    void commandHistoryUp();
    void commandHistoryDown();
    void clearCmd();

  private:
    UI ui_;

    entt::entity terminal_text_entt_ = entt::null;

    bool enabled_ = false;

    constexpr static int COMMAND_HISTORY_MAX_SIZE = 32;
    std::deque<std::u32string> command_history_;
    int current_command_idx_ = -1;

    KeyListenerHnd toggle_key_listener_;
    KeyListenerHnd enter_key_listener_;
    KeyListenerHnd up_key_listener_;
    KeyListenerHnd down_key_listener_;
    KeyListenerHnd clear_key_listener_;
};
}  // namespace mle::client
