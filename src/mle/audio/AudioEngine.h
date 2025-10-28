#pragma once

#include <thread>

#include "Types.h"
#include "mle/audio/Utils.h"
#include "mle/utils/ECS.h"
#include "mle/utils/Utils.h"
#include "mle/utils/containers/TSQueue.h"

namespace mle {
class VolumeMixer {
  public:
    [[nodiscard]] f32 compute(u8 b, f32 source_linear = 1.0F) const;
    void apply(ALuint src, u8 b, f32 source_linear = 1.0F) const { alSourcef(src, AL_GAIN, compute(b, source_linear)); }
    void setMasterDb(f32 db) { master_ = dbToLinear(db); }
    void setBusDb(u8 b, f32 db) {
        if (b < bus_volumes_db_.size()) {
            bus_volumes_db_.at(b) = db;
        }
    }

  private:
    f32 master_ = dbToLinear(0.0F);
    std::array<f32, 7> bus_volumes_db_{-6.0F};
};

class AudioEngine {
    MLE_SINGLETON(AudioEngine)
  private:
    struct OneShotSource {
        ALuint source{};
        u32 priority{0};
        u8 bus = 0;
    };

    struct Streaming {
        constexpr static usize BUFFER_COUNT = 4;
        constexpr static usize BUFFER_SIZE = 32768;
        constexpr static usize SAMPLES_PER_BUFFER = BUFFER_SIZE / sizeof(f32);

        std::array<ALuint, BUFFER_COUNT> buffers{0};
        ALuint source{0};
        usize queued_buffer_count{0};
        usize current_buffer{0};

        WavData wav{};
        usize current_sample{0};
        usize first_sample{0};
        usize last_sample{0};
        bool looping{false};
        u8 bus = 0;
        bool active{false};
    };

    constexpr static usize MAX_STREAMING_SOURCES = 6;

  public:
    Result init();
    void shutdown();

    Expected<std::vector<std::string>> getAvailableDevices();

    void enqueueCmd(const audio::Cmd& cmd);

  private:
    void runLoop(std::stop_token st);
    Result genSources(usize target = max<usize>());
    void updateSources();

    void stopStream(u8 id);
    static void fillStreamingBuffer(Streaming& stream);

    void processCmds();
    void processCmd(const audio::Cmd& cmd);

    void processCmdLoad(const audio::cmd::Load& cmd);
    void processCmdPlayOneShot(const audio::cmd::PlayOneShot& cmd);
    void processCmdStartStream(const audio::cmd::StartStream& cmd);
    void processCmdStop(const audio::cmd::StopStream& cmd);
    void processCmdPause(const audio::cmd::PauseStream& cmd);
    void processCmdResume(const audio::cmd::ResumeStream& cmd);
    void processCmdSetVolume(const audio::cmd::SetVolume& cmd);
    void processCmdSetListener(const audio::cmd::SetListener& cmd);
    void processCmdSetDistanceParams(const audio::cmd::SetDistanceParams& cmd);
    void processCmdStopAll(const audio::cmd::StopAll& cmd);

    static Result uploadToBuffer(ALuint buffer, const WavData& wav);

    void logAvailableDevices();

    void freeAllSources();

  private:
    ALCdevice* device_{};
    ALCcontext* context_{};

    VolumeMixer mixer_{};

    std::unordered_map<entt::id_type, ALuint> loaded_sounds_{};
    std::unordered_map<entt::id_type, Path> stream_sounds_{};

    std::vector<OneShotSource> one_shot_sources_{};

    std::array<Streaming, MAX_STREAMING_SOURCES> streaming_sources_{};

    std::jthread run_thread_{};

    // TODO: make this lock-free
    TSQueue<audio::Cmd> cmd_queue_{};

    // enum class RTCLTypes {LOAD, PLAY_ONE_SHOT, START_STREAM, STOP_STREAM, PAUSE_STREAM, RESUME_STREAM, SET_VOLUME, SET_LISTENER, SET_DISTANCE_PARAMS,
    // STOP_ALL};

    RuntimeConfigListener swapchain_rtcl0_;
    RuntimeConfigListener swapchain_rtcl1_;
    RuntimeConfigListener swapchain_default_clear_color_rtcl_;
    RuntimeConfigListener target_fps_rtcl_;
};
}  // namespace mle
