#include "Audio.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <dr_flac.h>

#include <list>
#include <map>
#include <memory>

#include "mle/common/Assert.h"
#include "mle/common/Logger.h"
#include "mle/common/containers/TSQueue.h"
#include "mle/common/math/Types.h"
#include "mle/core/Core.h"
#include "mle/lua/Lua.h"

namespace mle::audio {
namespace {
class Impl {
  public:
    void init();
    void shutdown();

    void update();

    void enqueueCommand(Command&& cmd) { command_queue_.push(std::move(cmd)); }

    static bool checkAL();
    bool checkALC();

  private:
    ALCdevice* device_ = nullptr;
    ALCcontext* context_ = nullptr;

    static constexpr size_t MAX_BUFFERS = 32;
    struct BufferEntry {
        ALuint buffer{};
        std::list<std::string>::iterator lru_it;
    };
    std::unordered_map<std::string, BufferEntry> buffer_cache_;
    std::list<std::string> lru_list_;
    TSQueue<Command> command_queue_;

    ALint music_source_{};
    ALint music_buffer_{};

    std::vector<ALuint> active_sources_;

    float master_volume_ = 1.0F;
    bool muted_ = false;
    float last_volume_before_mute_ = 1.0F;

    std::mutex al_mutex_;

    static ALuint loadFlacToBuffer(std::string filename);
    ALuint getBufferForSound(const std::string& name);
    void stopAllSources();
    void cleanupFinishedSources();

    void handleCommand(const PlaySound& cmd);
    void handleCommand(const PlayMusic& cmd);
    void handleCommand(const StopMusic& cmd) const;
    void handleCommand(const ResumeMusic& cmd) const;
    void handleCommand(const StopAll& cmd);
    void handleCommand(const Mute& cmd);
    void handleCommand(const Volume& cmd);
};

[[maybe_unused]] bool Impl::checkAL() {
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        MLE_E("OpenAL error {}", alGetString(error));
        return false;
    }
    return true;
}

bool Impl::checkALC() {
    ALCenum error = alcGetError(device_);
    if (error != ALC_NO_ERROR) {
        MLE_E("OpenALC error: {}", alcGetString(device_, error));
        return false;
    }
    return true;
}

void Impl::update() {
    core::threadPool().enqueue([this, cmds = command_queue_.popAll()]() {
        // This will probably cause me soume troubles in the future. But hey... Future me can deal with it.
        std::scoped_lock al_lock{al_mutex_};
        for (const auto& cmd : cmds) {
            std::visit([this](auto& c) { this->handleCommand(c); }, cmd);
        }
        cleanupFinishedSources();
    });
}

void Impl::init() {
    MLE_I("Initializing audio module. OpenAL");

    MLE_T("Init device.");
    device_ = alcOpenDevice(nullptr);
    if (!device_) {
        core::unrecoverable("Failed to open OpenAL device");
    }

    MLE_I("Selected audio device: {}", alcGetString(device_, ALC_DEVICE_SPECIFIER));

    context_ = alcCreateContext(device_, nullptr);
    if (!checkALC() || !context_) {
        core::unrecoverable("Failed to create OpenAL context");
    }

    auto context_made_current = alcMakeContextCurrent(context_);
    if (!checkALC() || !context_made_current) {
        core::unrecoverable("Failed to make OpenAL context current");
    }

    ALuint aux_music_source = 0;
    alGenSources(1, &aux_music_source);
    music_source_ = as<int>(aux_music_source);
    checkAL();
    alSourcef(music_source_, AL_GAIN, 1.0F);
    checkAL();
}

void Impl::shutdown() {
    MLE_I("Shutting down the audio module.");

    if (context_) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context_);
    }
    if (device_) {
        alcCloseDevice(device_);
    }
}

ALuint Impl::loadFlacToBuffer(std::string filename) {
    drflac_uint32 channels = 0;
    drflac_uint32 sample_rate = 0;
    drflac_uint64 total_frame_count = 0;

    filename.insert(0, "res/sounds/");

    i16* p_samples = drflac_open_file_and_read_pcm_frames_s16(filename.c_str(), &channels, &sample_rate, &total_frame_count, nullptr);

    if (!p_samples) {
        core::unrecoverable("Failed to open and decode FLAC file: {}", filename);
        return AL_NONE;
    }

    MLE_ASSERT(channels > 0 && channels <= 2);

    const usize total_samples = as<usize>(total_frame_count) * channels;
    ALenum format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

    MLE_T("Loaded sound file: {} | Channels: {} | Sample Rate: {} | Total Frames: {}", filename, channels, sample_rate, total_frame_count);

    ALuint buffer = 0;
    alGenBuffers(1, &buffer);
    checkAL();
    alBufferData(buffer, format, p_samples, as<int>(total_samples * sizeof(i16)), as<int>(sample_rate));
    checkAL();

    drflac_free(p_samples, nullptr);

    return buffer;
}

ALuint Impl::getBufferForSound(const std::string& name) {
    auto it = buffer_cache_.find(name);

    if (it != buffer_cache_.end()) {
        lru_list_.splice(lru_list_.begin(), lru_list_, it->second.lru_it);
        it->second.lru_it = lru_list_.begin();
        return it->second.buffer;
    }

    if (buffer_cache_.size() >= MAX_BUFFERS) {
        const std::string& lru_name = lru_list_.back();
        auto lru_it_map = buffer_cache_.find(lru_name);
        if (lru_it_map != buffer_cache_.end()) {
            ALuint old_buffer = lru_it_map->second.buffer;
            alDeleteBuffers(1, &old_buffer);
            buffer_cache_.erase(lru_it_map);
        }
        lru_list_.pop_back();
    }

    ALuint new_buffer = loadFlacToBuffer(name);
    if (new_buffer == AL_NONE) {
        core::unrecoverable("Failed to load sound buffer for: {}", name);
        return AL_NONE;
    }

    lru_list_.push_front(name);
    BufferEntry entry = {.buffer = new_buffer, .lru_it = lru_list_.begin()};
    buffer_cache_[name] = entry;
    return new_buffer;
}

void Impl::stopAllSources() {
    for (ALuint src : active_sources_) {
        alSourceStop(src);
        checkAL();
        alDeleteSources(1, &src);
        checkAL();
    }
    active_sources_.clear();
    alSourcePause(music_source_);
    checkAL();
}

void Impl::cleanupFinishedSources() {
    auto it = active_sources_.begin();
    while (it != active_sources_.end()) {
        ALint state = 0;
        alGetSourcei(*it, AL_SOURCE_STATE, &state);
        checkAL();
        if (state == AL_STOPPED) {
            alDeleteSources(1, &(*it));
            checkAL();
            it = active_sources_.erase(it);
        } else {
            ++it;
        }
    }
}

void Impl::handleCommand(const PlaySound& cmd) {
    auto buffer = getBufferForSound(cmd.name);

    if (buffer == AL_NONE) {
        return;
    }

    ALuint source = 0;
    alGenSources(1, &source);
    checkAL();

    alSourcef(source, AL_GAIN, 1.0F);
    alSource3f(source, AL_POSITION, 0.0F, 0.0F, 0.0F);
    alSource3f(source, AL_VELOCITY, 0.0F, 0.0F, 0.0F);
    alSource3f(source, AL_DIRECTION, 0.0F, 0.0F, 0.0F);
    alSourcef(source, AL_PITCH, 1.0F);
    alSourcei(source, AL_LOOPING, AL_FALSE);
    checkAL();

    alSourcei(source, AL_BUFFER, as<int>(buffer));
    checkAL();

    alSourcePlay(source);
    checkAL();

    active_sources_.push_back(source);
}

void Impl::handleCommand(const PlayMusic& cmd) {
    if (music_buffer_ != AL_NONE) {
        alSourceStop(music_source_);
        checkAL();
        ALuint uintbuffer = music_buffer_;
        alDeleteBuffers(1, &uintbuffer);
        checkAL();
        music_buffer_ = AL_NONE;
    }

    // FIXME: stream this
    music_buffer_ = as<int>(loadFlacToBuffer(cmd.name));
    if (music_buffer_ == AL_NONE) {
        core::unrecoverable("Failed to load music buffer for: {}", cmd.name);
        return;
    }

    alSourcei(music_source_, AL_BUFFER, music_buffer_);
    checkAL();
    alSourcei(music_source_, AL_LOOPING, AL_FALSE);
    checkAL();
    alSourcePlay(music_source_);
    checkAL();
}

void Impl::handleCommand(const StopMusic& /*unused*/) const {
    alSourcePause(music_source_);
    checkAL();
}

void Impl::handleCommand(const ResumeMusic& /*unused*/) const {
    ALint state = 0;
    alGetSourcei(music_source_, AL_SOURCE_STATE, &state);
    checkAL();
    if (state == AL_PAUSED) {
        alSourcePlay(music_source_);
        checkAL();
    }
}

void Impl::handleCommand(const StopAll& /*unused*/) {
    stopAllSources();
}

void Impl::handleCommand(const Mute& /*unused*/) {
    if (!muted_) {
        alGetListenerf(AL_GAIN, &last_volume_before_mute_);
        checkAL();
        alListenerf(AL_GAIN, 0.0F);
        checkAL();
        muted_ = true;
    } else {
        alListenerf(AL_GAIN, last_volume_before_mute_);
        checkAL();
        muted_ = false;
    }
}

void Impl::handleCommand(const Volume& cmd) {
    if (cmd.type == 0) {
        f32 vol = std::max(0.0F, std::min(1.0F, as<f32>(cmd.val) / 100.0F));
        master_volume_ = vol;
        alListenerf(AL_GAIN, vol);
        checkAL();
        muted_ = false;
    }
    // TODO: handle other volume types
}

// TODO: I will probably allocate this at a linear allocator along the other core singletons in the future
std::unique_ptr<Impl> i_;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
};  // namespace

void init() {
    MLE_ASSERT(!i_);
    i_ = std::make_unique<Impl>();
    i_->init();
}

void shutdown() {
    MLE_ASSERT(i_);
    i_->shutdown();
    i_.reset();
}

void update() {
    MLE_ASSERT(i_);
    i_->update();
}

void enqueueCommand(Command cmd) {
    MLE_ASSERT(i_);
    i_->enqueueCommand(std::move(cmd));
}

void registerLuaTypes() {
    MLE_T("Registering audio Lua types");

    auto audio_table = lua::createTable();
    audio_table["play"] = [](const std::string& name) { enqueueCommand(PlaySound{.name = name}); };

    auto mle_table = lua::getMleTable();
    mle_table["audio"] = audio_table;
}
}  // namespace mle::audio
