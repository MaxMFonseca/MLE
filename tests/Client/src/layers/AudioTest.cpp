#include "AudioTest.h"

#include <algorithm>

#include "Init.h"
#include "mle/audio/AudioEngine.h"
#include "mle/audio/Types.h"
#include "mle/client/Client.h"
#include "mle/renderer/Renderer.h"

namespace mle::user {
namespace {
constexpr f32 FIXED_DT_SECONDS = 1.0F / 60.0F;
constexpr u8 STREAM_SLOT = 5;
constexpr u32 PROTECTION_FILLER_COUNT = 249;
constexpr std::array CUE_SOUNDS{
    "i/generated/combat_high",
    "i/generated/combat_low",
    "i/generated/hit_03",
    "i/generated/hit_04",
};

entt::id_type soundId(const std::string& name) {
    return entt::hashed_string::value(name.c_str());
}
}  // namespace

void AudioTestLayer::preloadFixtures() {
    constexpr std::array ONE_SHOTS{
        "i/generated/combat_high", "i/generated/combat_low", "i/generated/ui_protected", "i/generated/protection_filler", "i/generated/hit_01",
        "i/generated/hit_02",      "i/generated/hit_03",     "i/generated/hit_04",       "i/generated/duration_mono",     "i/generated/duration_stereo",
    };
    for (const char* name : ONE_SHOTS) {
        AudioEngine::i().enqueueCmd(audio::cmd::Load{.name = name, .stream = false});
    }
    AudioEngine::i().enqueueCmd(audio::cmd::Load{.name = "i/generated/combat_texture", .stream = true});
    AudioEngine::i().enqueueCmd(audio::cmd::Load{.name = "i/generated/duration_mono", .stream = true});
    AudioEngine::i().enqueueCmd(audio::cmd::Load{.name = "i/generated/duration_stereo", .stream = true});
}

void AudioTestLayer::play(const std::string& name, u32 priority_override) {
    AudioEngine::i().enqueueCmd(audio::cmd::PlayOneShot{
        .sound_id = soundId(name),
        .priority = priority_override == 0 ? priority_ : priority_override,
        .params = {.bus = bus_},
    });
}

void AudioTestLayer::startDurationStream(const std::string& name, u8 slot, u32 offset_ms, u32 duration_ms) {
    AudioEngine::i().enqueueCmd(audio::cmd::StartStream{.sound_id = soundId("i/generated/" + name),
                                                        .loop = false,
                                                        .id = slot,
                                                        .params = {.start_offset_ms = offset_ms, .duration_ms = duration_ms, .bus = bus_}});
}

void AudioTestLayer::startProtectionPhase(bool protected_phase) {
    AudioEngine::i().enqueueCmd(audio::cmd::StopAll{});
    protection_phase_ = protected_phase;
    scenario_ = protected_phase ? "UI protection: protected phase" : "UI protection: unprotected phase";
    AudioEngine::i().enqueueCmd(audio::cmd::SetBusVoicePolicy{.bus = 0});
    AudioEngine::i().enqueueCmd(audio::cmd::SetBusVoicePolicy{.bus = 1, .max_voices = 1, .protected_from_other_buses = protected_phase});
    AudioEngine::i().enqueueCmd(audio::cmd::PlayOneShot{.sound_id = soundId("i/generated/ui_protected"), .priority = 1, .params = {.bus = 1}});
    preset_combat_remaining_ = PROTECTION_FILLER_COUNT;
    pending_action_ = PendingAction::COMBAT_STEAL_ATTEMPT;
}

void AudioTestLayer::applyPolicy() {
    AudioEngine::i().enqueueCmd(audio::cmd::SetBusVoicePolicy{.bus = bus_, .max_voices = cap_, .protected_from_other_buses = protected_});
}

void AudioTestLayer::resetTelemetry() {
    raw_ = aggregated_ = submitted_ = dropped_ = 0;
    event_accumulator_ = 0.0F;
}

void AudioTestLayer::resetScenario() {
    sustained_ = false;
    AudioEngine::i().enqueueCmd(audio::cmd::StopAll{.fade_out_ms = 0});
    for (u8 bus = 0; bus < audio::BUS_COUNT; ++bus) {
        AudioEngine::i().enqueueCmd(audio::cmd::SetBusVoicePolicy{.bus = bus});
    }

    aggregator_ = {};
    bus_ = 0;
    priority_ = 2;
    cap_ = 8;
    protected_ = false;
    rate_ = 60;
    stream_volume_ = 0.65F;
    stream_pitch_ = 1.0F;
    stream_fade_in_ms_ = 250;
    stream_fade_out_ms_ = 500;
    stream_ramp_fade_ms_ = 250;
    stream_duration_ms_ = 0;
    scenario_ = "Idle";
    pending_action_ = PendingAction::NONE;
    pending_frames_ = 0;
    preset_combat_remaining_ = 0;
    protection_phase_ = false;
    resetTelemetry();
    applyPolicy();
}

void AudioTestLayer::loadPreset(const std::string& preset) {
    sustained_ = false;
    pending_action_ = PendingAction::NONE;
    pending_frames_ = 0;
    preset_combat_remaining_ = 0;
    scenario_ = preset;
    AudioEngine::i().enqueueCmd(audio::cmd::StopAll{});

    const auto policy = [](u8 bus, u16 cap, bool protected_from_other_buses = false) {
        AudioEngine::i().enqueueCmd(audio::cmd::SetBusVoicePolicy{.bus = bus, .max_voices = cap, .protected_from_other_buses = protected_from_other_buses});
    };
    const auto shot = [](const char* name, u32 priority, u8 bus) {
        AudioEngine::i().enqueueCmd(audio::cmd::PlayOneShot{.sound_id = entt::hashed_string::value(name), .priority = priority, .params = {.bus = bus}});
    };

    if (preset == "Cap rejection") {
        bus_ = 0;
        cap_ = 1;
        protected_ = false;
        policy(0, 1);
        shot("i/generated/combat_low", 4, 0);
        shot("i/generated/combat_high", 1, 0);
    } else if (preset == "Equal-priority rejection") {
        bus_ = 0;
        cap_ = 1;
        protected_ = false;
        policy(0, 1);
        shot("i/generated/hit_01", 3, 0);
        shot("i/generated/hit_02", 3, 0);
    } else if (preset == "Higher-priority replacement") {
        bus_ = 0;
        cap_ = 1;
        protected_ = false;
        policy(0, 1);
        shot("i/generated/combat_low", 1, 0);
        shot("i/generated/combat_high", 4, 0);
    } else if (preset == "UI protection") {
        bus_ = 1;
        cap_ = 1;
        protected_ = false;
        startProtectionPhase(false);
    } else if (preset == "Texture fade") {
        bus_ = 0;
        stream_volume_ = 0.8F;
        stream_pitch_ = 1.0F;
        stream_fade_in_ms_ = 750;
        stream_fade_out_ms_ = 1000;
        stream_duration_ms_ = 3000;
        AudioEngine::i().enqueueCmd(audio::cmd::StartStream{
            .sound_id = soundId("i/generated/combat_texture"),
            .loop = true,
            .id = STREAM_SLOT,
            .params = {.volume = stream_volume_, .pitch = stream_pitch_, .duration_ms = stream_duration_ms_, .fade_in_ms = stream_fade_in_ms_, .bus = bus_}});
        pending_action_ = PendingAction::STOP_TEXTURE;
        pending_frames_ = 120;
    } else if (preset == "Ramp replacement") {
        stream_volume_ = 1.0F;
        stream_pitch_ = 1.2F;
        stream_ramp_fade_ms_ = 900;
        AudioEngine::i().enqueueCmd(audio::cmd::StartStream{
            .sound_id = soundId("i/generated/combat_texture"), .loop = true, .id = STREAM_SLOT, .params = {.volume = 0.2F, .bus = bus_}});
        AudioEngine::i().enqueueCmd(audio::cmd::SetStreamParams{.id = STREAM_SLOT, .volume = 0.4F, .pitch = 0.8F, .fade_ms = 1500});
        pending_action_ = PendingAction::REPLACE_RAMP;
        pending_frames_ = 30;
    } else if (preset == "Sparse") {
        rate_ = 15;
    } else if (preset == "Battle") {
        rate_ = 120;
    } else if (preset == "Saturation") {
        rate_ = 600;
    } else if (preset == "Corrupt fixture") {
        AudioEngine::i().enqueueCmd(audio::cmd::Load{.name = "i/generated/corrupt", .stream = false});
    }
}

void AudioTestLayer::init() {
    MLE_I("AudioTestLayer::init()");
    preloadFixtures();
    applyPolicy();

    auto& game = Client::i().getGameLayerTable();
    game["ui"] = &ui_;
    game["return_to_init"] = []() { Client::i().pushGameLayer(std::make_unique<InitLayer>()); };
    game["audio_test_play"] = [this](const std::string& name) { play(name); };
    game["audio_test_trigger_ui"] = [this]() { play("i/generated/ui_protected", 4); };
    game["audio_test_set_bus"] = [this](u8 value) {
        bus_ = std::min<u8>(value, audio::BUS_COUNT - 1);
        applyPolicy();
    };
    game["audio_test_set_priority"] = [this](u32 value) { priority_ = std::clamp(value, 1U, 4U); };
    game["audio_test_set_policy"] = [this](u16 cap, bool value) {
        cap_ = cap;
        protected_ = value;
        applyPolicy();
    };
    game["audio_test_load_preset"] = [this](const std::string& preset) { loadPreset(preset); };
    game["audio_test_start_load"] = [this]() {
        // PlayOneShot producers are mutually exclusive. Each active producer
        // submits at most four commands per fixed update.
        preset_combat_remaining_ = 0;
        pending_action_ = PendingAction::NONE;
        pending_frames_ = 0;
        sustained_ = true;
    };
    game["audio_test_stop_load"] = [this]() { sustained_ = false; };
    game["audio_test_set_rate"] = [this](u32 value) { rate_ = std::min(value, 20'000U); };
    game["audio_test_stream_start"] = [this]() {
        AudioEngine::i().enqueueCmd(audio::cmd::StartStream{
            .sound_id = soundId("i/generated/combat_texture"),
            .loop = true,
            .id = STREAM_SLOT,
            .params = {.volume = stream_volume_, .pitch = stream_pitch_, .duration_ms = stream_duration_ms_, .fade_in_ms = stream_fade_in_ms_, .bus = bus_}});
    };
    game["audio_test_stream_stop"] = [this]() { AudioEngine::i().enqueueCmd(audio::cmd::StopStream{.id = STREAM_SLOT, .fade_out_ms = stream_fade_out_ms_}); };
    game["audio_test_stream_pause"] = []() { AudioEngine::i().enqueueCmd(audio::cmd::PauseStream{.id = STREAM_SLOT}); };
    game["audio_test_stream_resume"] = []() { AudioEngine::i().enqueueCmd(audio::cmd::ResumeStream{.id = STREAM_SLOT}); };
    game["audio_test_stream_params"] = [this](f32 volume, f32 pitch, u32 fade_ms) {
        stream_volume_ = std::clamp(volume, 0.0F, 2.0F);
        stream_pitch_ = std::clamp(pitch, 0.25F, 4.0F);
        stream_ramp_fade_ms_ = fade_ms;
        AudioEngine::i().enqueueCmd(audio::cmd::SetStreamParams{.id = STREAM_SLOT, .volume = stream_volume_, .pitch = stream_pitch_, .fade_ms = fade_ms});
    };
    game["audio_test_set_stream_fades"] = [this](u32 fade_in_ms, u32 fade_out_ms) {
        stream_fade_in_ms_ = fade_in_ms;
        stream_fade_out_ms_ = fade_out_ms;
    };
    game["audio_test_set_stream_duration"] = [this](u32 duration_ms) { stream_duration_ms_ = duration_ms; };
    game["audio_test_duration"] = [this](const std::string& name, u8 slot, u32 offset_ms, u32 duration_ms) {
        startDurationStream(name, slot, offset_ms, duration_ms);
    };
    game["audio_test_stop_all"] = [this]() {
        sustained_ = false;
        pending_action_ = PendingAction::NONE;
        pending_frames_ = 0;
        preset_combat_remaining_ = 0;
        AudioEngine::i().enqueueCmd(audio::cmd::StopAll{});
    };
    game["audio_test_reset"] = [this]() { resetScenario(); };
    game["audio_test_status"] = [this]() {
        auto status = Client::i().lua().createTable();
        status["raw"] = raw_;
        status["aggregated"] = aggregated_;
        status["submitted"] = submitted_;
        status["dropped"] = dropped_;
        status["bus"] = bus_;
        status["priority"] = priority_;
        status["cap"] = cap_;
        status["protected"] = protected_;
        status["rate"] = rate_;
        status["scenario"] = scenario_;
        status["running"] = sustained_;
        status["stream_volume"] = stream_volume_;
        status["stream_pitch"] = stream_pitch_;
        status["stream_duration_ms"] = stream_duration_ms_;
        status["stream_fade_in_ms"] = stream_fade_in_ms_;
        status["stream_fade_out_ms"] = stream_fade_out_ms_;
        status["stream_ramp_fade_ms"] = stream_ramp_fade_ms_;
        return status;
    };

    ui_.setRoot("i/ui/AudioTestLayer");
}

void AudioTestLayer::submitSustainedFrame() {
    event_accumulator_ += static_cast<f32>(rate_) * FIXED_DT_SECONDS;
    const u32 accepted = static_cast<u32>(event_accumulator_);
    event_accumulator_ -= static_cast<f32>(accepted);
    const auto result = aggregator_.advance(FIXED_DT_SECONDS, accepted);
    raw_ += result.raw;
    aggregated_ += result.aggregated;
    dropped_ += result.dropped;
    submitted_ += result.submitted;
    for (u32 i = 0; i < result.submitted; ++i) {
        const auto& cue = result.cues.at(i);
        AudioEngine::i().enqueueCmd(audio::cmd::PlayOneShot{
            .sound_id = soundId(CUE_SOUNDS.at(static_cast<usize>(cue.kind))),
            .priority = cue.priority,
            .params = {.bus = bus_},
        });
    }
}

void AudioTestLayer::update() {
    // Producer exclusivity keeps fixed-update PlayOneShot submission <= 4.
    if (sustained_) {
        submitSustainedFrame();
    }
    if (preset_combat_remaining_ != 0) {
        const u32 count = std::min(preset_combat_remaining_, 4U);
        for (u32 i = 0; i < count; ++i) {
            AudioEngine::i().enqueueCmd(audio::cmd::PlayOneShot{
                .sound_id = soundId("i/generated/protection_filler"),
                .priority = 2,
                .params = {.bus = 0},
            });
        }
        preset_combat_remaining_ -= count;
        if (preset_combat_remaining_ == 0) {
            pending_frames_ = 2;
        }
    }
    const bool pending_action_ready = pending_frames_ != 0;
    if (pending_action_ready) {
        --pending_frames_;
    }
    if (pending_action_ready && pending_frames_ == 0) {
        if (pending_action_ == PendingAction::STOP_TEXTURE) {
            AudioEngine::i().enqueueCmd(audio::cmd::StopStream{.id = STREAM_SLOT, .fade_out_ms = stream_fade_out_ms_});
        } else if (pending_action_ == PendingAction::REPLACE_RAMP) {
            AudioEngine::i().enqueueCmd(
                audio::cmd::SetStreamParams{.id = STREAM_SLOT, .volume = stream_volume_, .pitch = stream_pitch_, .fade_ms = stream_ramp_fade_ms_});
        } else if (pending_action_ == PendingAction::COMBAT_STEAL_ATTEMPT) {
            AudioEngine::i().enqueueCmd(audio::cmd::PlayOneShot{.sound_id = soundId("i/generated/combat_high"), .priority = 4, .params = {.bus = 0}});
            if (!protection_phase_) {
                pending_action_ = PendingAction::START_PROTECTED_PHASE;
                pending_frames_ = 90;
                ui_.update();
                return;
            }
        } else if (pending_action_ == PendingAction::START_PROTECTED_PHASE) {
            protected_ = true;
            startProtectionPhase(true);
            ui_.update();
            return;
        }
        pending_action_ = PendingAction::NONE;
    }
    ui_.update();
}

ImageRef AudioTestLayer::render() {
    auto* image = render_target_.getImage();
    if (auto* ui_image = ui_.render()) {
        image->blend(Renderer::i().frameRenderer().cmd(), *ui_image);
    }
    return image;
}

void AudioTestLayer::shutdown() {
    sustained_ = false;
    pending_action_ = PendingAction::NONE;
    pending_frames_ = 0;
    preset_combat_remaining_ = 0;
    AudioEngine::i().enqueueCmd(audio::cmd::StopAll{.fade_out_ms = 0});
    ui_.shutdown();
}
}  // namespace mle::user
