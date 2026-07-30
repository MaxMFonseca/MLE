#pragma once

#include <array>
#include <string>

#include "../audio/CombatEventAggregator.h"
#include "mle/client/Layer.h"
#include "mle/ui/UI.h"

namespace mle::user {
class AudioTestLayer : public mle::client::Layer {
  public:
    MLE_NO_COPY_MOVE(AudioTestLayer)

    AudioTestLayer() = default;
    ~AudioTestLayer() override = default;

    void init() override;
    void shutdown() override;
    void update() override;
    ImageRef render() override;

  private:
    enum class PendingAction : u8 { NONE, STOP_TEXTURE, REPLACE_RAMP, COMBAT_STEAL_ATTEMPT, START_PROTECTED_PHASE };
    static void preloadFixtures();
    void submitSustainedFrame();
    void play(const std::string& name, u32 priority_override = 0);
    void startDurationStream(const std::string& name, u8 slot, u32 offset_ms, u32 duration_ms);
    void startProtectionPhase(bool protected_phase);
    void applyPolicy();
    void resetTelemetry();
    void resetScenario();
    void loadPreset(const std::string& preset);

    UI ui_;
    mle::client::WindowSizedRenderTarget render_target_;
    audio::CombatEventAggregator aggregator_;
    bool sustained_{};
    f32 event_accumulator_{};
    u8 bus_{};
    u32 priority_{2};
    u16 cap_{8};
    bool protected_{};
    u32 rate_{60};
    f32 stream_volume_{0.65F};
    f32 stream_pitch_{1.0F};
    u32 stream_fade_in_ms_{250};
    u32 stream_fade_out_ms_{500};
    u32 stream_ramp_fade_ms_{250};
    u32 stream_duration_ms_{};
    u64 raw_{};
    u64 aggregated_{};
    u64 submitted_{};
    u64 dropped_{};
    std::string scenario_{"Idle"};
    PendingAction pending_action_{PendingAction::NONE};
    u32 pending_frames_{};
    u32 preset_combat_remaining_{};
    bool protection_phase_{};
};
}  // namespace mle::user
