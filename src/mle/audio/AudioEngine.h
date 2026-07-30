#pragma once

#include <memory>
#include <optional>
#include <span>
#include <thread>

#include "Types.h"
#include "mle/audio/AudioThreadStartup.h"
#include "mle/audio/CommandMailbox.h"
#include "mle/audio/SourcePlaybackState.h"
#include "mle/audio/StreamGroup.h"
#include "mle/audio/StreamState.h"
#include "mle/audio/Utils.h"
#include "mle/audio/VoiceAllocator.h"
#include "mle/utils/ECS.h"
#include "mle/utils/Utils.h"
#include "mle/utils/containers/TSQueue.h"

namespace mle {
class VolumeMixer {
  public:
  public:
  private:
};

class AudioEngine {
    MLE_SINGLETON(AudioEngine)
  private:
    struct OneShotSource {
        ALuint source{};
    };

    struct Streaming {
        static constexpr usize BUFFER_COUNT = 4;
        static constexpr usize BUFFER_SIZE = 32768;
        static constexpr usize SAMPLES_PER_BUFFER = BUFFER_SIZE / sizeof(f32);

        std::array<ALuint, BUFFER_COUNT> buffers{0};
        ALuint source{0};
        usize queued_buffer_count{0};
        usize current_buffer{0};

        std::shared_ptr<const WavData> wav{};
        usize current_sample{0};
        usize first_sample{0};
        usize last_sample{0};
        bool looping{false};
        u8 bus = 0;
        f32 volume{1.0F};
        audio::VolumeRamp ramp{};
        std::chrono::steady_clock::time_point ramp_tick{};
        bool active{false};
        bool cleanup_pending{false};
        bool paused{false};
    };

    struct StreamStartPlan {
        audio::cmd::StartStream cmd;
        std::shared_ptr<const WavData> wav;
        audio::ValidatedStreamStart validated;
    };

    static constexpr usize CMD_QUEUE_SIZE = 128;
    static constexpr usize BUS_COUNT = 8;

  public:
    Result init();
    void shutdown();

    Expected<std::vector<std::string>> getAvailableDevices();

    void enqueueCmd(const audio::Cmd& cmd);

    f32 getVolume(u8 bus) { return bus < 8 ? bus_volumes_.at(bus) : 0.0F; }

    static void addLuaBinding();

  private:
    void initRTCLs();

    void runLoop(std::stop_token st);
    Result genSources(usize target = max<usize>());
    void updateSources();

    Expected<StreamStartPlan> preflightStreamStart(const audio::cmd::StartStream& cmd) const;
    Result prepareStream(const StreamStartPlan& plan);
    void clearStreamTargets(std::span<const u8> slots);

    Result stopStream(u8 id);
    static Result fillStreamingBuffer(Streaming& stream);

    void processCmds();
    void processCmd(const audio::Cmd& cmd);

    void processCmdLoad(const audio::cmd::Load& cmd);
    void processCmdPlayOneShot(const audio::cmd::PlayOneShot& cmd);
    void processCmdStartStream(const audio::cmd::StartStream& cmd);
    void processCmdStartStreamGroup(const audio::cmd::StartStreamGroup& cmd);
    void processCmdStop(const audio::cmd::StopStream& cmd);
    void processCmdSetStreamParams(const audio::cmd::SetStreamParams& cmd);
    void processCmdPause(const audio::cmd::PauseStream& cmd);
    void processCmdResume(const audio::cmd::ResumeStream& cmd);
    void processCmdSetVolume(const audio::cmd::SetVolume& cmd);
    void processCmdSetListener(const audio::cmd::SetListener& cmd);
    void processCmdSetDistanceParams(const audio::cmd::SetDistanceParams& cmd);
    void processCmdStopAll(const audio::cmd::StopAll& cmd);
    void processCmdSetBusVoicePolicy(const audio::cmd::SetBusVoicePolicy& cmd);

    static Result uploadToBuffer(ALuint buffer, const WavData& wav);

    void logAvailableDevices();

    void freeAllSources();

    void setBusVolumeLinear(u8 b, f32 linear);
    Result applyVolume(ALuint source, u8 bus, f32 source_linear) const;
    static Result applySourcePlaybackState(ALuint source, const audio::SourcePlaybackState& state);

  private:
    ALCdevice* device_{};
    ALCcontext* context_{};

    // I am reading from this from multiple threads, but only writing from one.
    // Maybe needs a mutex? for proper sync on ui later?
    std::array<f32, BUS_COUNT> bus_volumes_{};

    std::unordered_map<entt::id_type, ALuint> loaded_sounds_{};
    std::unordered_map<entt::id_type, std::shared_ptr<const WavData>> stream_sounds_{};

    std::vector<OneShotSource> one_shot_sources_{};
    std::vector<audio::VoiceMetadata> one_shot_voice_metadata_{};
    std::array<audio::BusVoicePolicy, audio::BUS_COUNT> bus_voice_policies_{};

    std::array<Streaming, audio::STREAM_SLOT_COUNT> streaming_sources_{};

    std::jthread run_thread_{};

    audio::CommandMailbox cmd_mailbox_{CMD_QUEUE_SIZE};
    audio::AudioThreadStartup thread_startup_{};

    enum class RTCLs : u8 { PLAY_ONE_SHOT, START_STREAM, STOP_STREAM, PAUSE_STREAM, RESUME_STREAM, SET_VOLUME, STOP_ALL, COUNT };

    std::array<RuntimeConfigListener, as<usize>(RTCLs::COUNT)> rtcls_;
};
}  // namespace mle
