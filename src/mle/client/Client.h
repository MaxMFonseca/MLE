#pragma once

#include <map>
#include <memory>
#include <vector>

#include "Layer.h"
#include "mle/lua/Lua.h"
#include "mle/utils/Stopwatch.h"
#include "mle/utils/SystemState.h"
#include "mle/utils/Utils.h"
#include "mle/window/Events.h"

namespace mle {
class Client final {
    MLE_SINGLETON(Client)

  public:
    struct TimeStats {
        struct Tracked {
            int updates = 0;
            int frames = 0;
            f32 time_updating_ms = 0;
            f32 time_rendering_ms = 0;
            std::map<std::string, f32> other_times_ms_;
        };

        Tracked current_second;
        Tracked last_second;
        Tracked all_time;

        int seconds = 0;
    };

  public:
    void init();
    void run();
    void requestStop();

    void pushGameLayer(std::unique_ptr<client::Layer> layer);
    void popGameLayer();

    void addDebugLayer(const std::string& name, std::unique_ptr<client::Layer> layer);
    void removeDebugLayer(const std::string& name);
    void suspendDebugLayer(const std::string& name);

    void accumulateTime(const std::string& key, f32 time);

    [[nodiscard]] const auto& getTimeStats() const { return time_; }
    [[nodiscard]] const auto& getRunningStopwatch() const { return running_sw_; }
    void logTimeStatsLastSecond() const;
    void logTimeStatsAllTime() const;

  private:
    void update();
    void updateTimeStatsEachSecond();
    void render(f64 dt);
    void shutdown();

  private:
    SystemState state_ = SystemState::UNINITIALIZED;

    Lua lua_;
    sol::table client_table_;

    std::vector<std::move_only_function<void()>> on_shutdown_callbacks_;

    std::unique_ptr<client::Layer> core_layer_;
    std::unique_ptr<client::Layer> next_layer_;
    std::map<std::string, std::unique_ptr<client::Layer>> debug_layers_;

    TimeStats time_{};
    Stopwatch running_sw_{};

    window::ev::CloseL window_close_el_;
};
}  // namespace mle

namespace fmt {
template <>
struct formatter<mle::Client::TimeStats::Tracked> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::Client::TimeStats::Tracked& stats, FormatContext& ctx) const {
        format_to(ctx.out(), "Updates: {}, Time Updating: {:.3f}ms | Frames: {}, Time Rendering: {:.3f}ms", stats.updates, stats.time_updating_ms, stats.frames,
                  stats.time_rendering_ms);
        for (const auto& [key, value] : stats.other_times_ms_) {
            format_to(ctx.out(), " | {}: {:.3f}ms", key, value);
        }
        return ctx.out();
    }
};
}  // namespace fmt
