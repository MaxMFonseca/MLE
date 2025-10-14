#pragma once

#include <map>
#include <memory>
#include <vector>

#include "Layer.h"
#include "mle/lua/Lua.h"
#include "mle/renderer/Types.h"
#include "mle/utils/Stopwatch.h"
#include "mle/utils/SystemState.h"
#include "mle/utils/Utils.h"
#include "mle/window/Events.h"
#include "mle/window/UserInputManager.h"

namespace mle {
class Client final {
    MLE_SINGLETON(Client)

  public:
  public:
    void init();
    void run();
    void requestStop();

    void pushGameLayer(std::unique_ptr<client::Layer> layer);
    void popGameLayer();

    void addDebugLayer(const std::string& name, std::unique_ptr<client::Layer> layer);
    void removeDebugLayer(const std::string& name);
    void suspendDebugLayer(const std::string& name);

    [[nodiscard]] const auto& getRunningStopwatch() const { return running_sw_; }

    ImageRef render() { return nullptr; }

  private:
    void update();
    void shutdown();

  private:
    SystemState state_ = SystemState::UNINITIALIZED;

    Lua lua_;
    sol::table client_table_;

    std::vector<std::move_only_function<void()>> on_shutdown_callbacks_;

    std::unique_ptr<client::Layer> core_layer_;
    std::unique_ptr<client::Layer> next_layer_;
    std::map<std::string, std::unique_ptr<client::Layer>> debug_layers_;

    Stopwatch running_sw_{};

    window::ev::CloseL window_close_el_;
};
}  // namespace mle
