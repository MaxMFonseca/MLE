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
    [[nodiscard]] float compute(u8 b, float source_linear = 1.0F) const;
    void apply(ALuint src, u8 b, float source_linear = 1.0F) const { alSourcef(src, AL_GAIN, compute(b, source_linear)); }
    void setMasterDb(float db) { master_ = dbToLinear(db); }
    void setBusDb(u8 b, float db) {
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
    struct Source {
        ALuint id{};
        u32 priority{0};
    };

  public:
    Result init();
    void shutdown();

    Expected<std::vector<std::string>> getAvailableDevices();

    ID enqueueCmd(const audio::Cmd& cmd);

  private:
    void runLoop(std::stop_token st);
    void genSources(usize target = 32);
    void updateSources();

    void processCmds();
    void processCmd(const audio::Cmd& cmd, ID cmd_id);

    void processCmdLoad(const audio::cmd::Load& cmd, ID cmd_id);
    void processCmdPlay(const audio::cmd::Play& cmd, ID cmd_id);
    void processCmdStop(const audio::cmd::Stop& cmd, ID cmd_id);
    void processCmdPause(const audio::cmd::Pause& cmd, ID cmd_id);
    void processCmdResume(const audio::cmd::Resume& cmd, ID cmd_id);
    void processCmdSetVolume(const audio::cmd::SetVolume& cmd, ID cmd_id);
    void processCmdSetListener(const audio::cmd::SetListener& cmd, ID cmd_id);
    void processCmdSetDistanceParams(const audio::cmd::SetDistanceParams& cmd, ID cmd_id);
    void processCmdStopAll(const audio::cmd::StopAll& cmd, ID cmd_id);

    static Result uploadToBuffer(ALuint buffer, const WavData& wav);

    void logAvailableDevices();

  private:
    ALCdevice* device_{};
    ALCcontext* context_{};

    VolumeMixer mixer_{};

    std::unordered_map<entt::id_type, ALuint> loaded_sounds_{};
    std::unordered_map<entt::id_type, std::string> stream_sounds_{};

    std::vector<Source> sources_{};

    std::jthread run_thread_{};

    TSQueue<std::pair<audio::Cmd, ID>> cmd_queue_{};
};
}  // namespace mle
