#pragma once

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "Layer.h"
#include "mle/lua/Lua.h"
#include "mle/renderer/Types.h"
#include "mle/utils/ColorCache.h"
#include "mle/utils/Stopwatch.h"
#include "mle/utils/SystemState.h"
#include "mle/utils/Utils.h"
#include "mle/window/Events.h"
#include "mle/window/UserInputManager.h"

namespace mle {
class Client final {
    MLE_SINGLETON(Client)
  public:
    void init();
    void run();
    void requestStop();

    void pushGameLayer(std::unique_ptr<client::Layer>&& layer) { next_game_layer_ = std::move(layer); }

    void addDebugLayer(const std::string& name, std::unique_ptr<client::Layer> layer);
    void removeDebugLayer(const std::string& name);

    [[nodiscard]] const auto& getRunningStopwatch() const { return running_sw_; }

    ImageRef render();

    auto& lua() { return lua_; }
    auto& getCTable() { return client_table_; }
    auto& getGameLayerTable() { return game_layer_table_; }

  private:
    void update();
    void shutdown();
    void checkNextGameLayer();

    void addPerfLayer();
    void addTerminalLayer();

  private:
    SystemState state_ = SystemState::UNINITIALIZED;

    Lua lua_;
    sol::table client_table_;
    sol::table game_layer_table_;

    std::mutex game_layer_render_mutex_;

    std::unique_ptr<client::Layer> game_layer_;
    std::unique_ptr<client::Layer> next_game_layer_;

    std::mutex debug_layers_render_mutex_;

    std::map<std::string, std::unique_ptr<client::Layer>> debug_layers_;
    std::set<std::string> debug_layers_to_remove_;

    Stopwatch running_sw_{};

    ColorCache color_cache_{};

    window::ev::CloseL window_close_el_;
};
}  // namespace mle
