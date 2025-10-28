#include "AudioEngine.h"

#include <AL/al.h>
#include <AL/alext.h>

#include <algorithm>
#include <expected>
#include <source_location>

#include "mle/audio/Types.h"
#include "mle/audio/Utils.h"
#include "mle/core/Assert.h"
#include "mle/core/Consts.h"
#include "mle/core/Logger.h"
#include "mle/core/PerfTracker.h"

namespace mle {
namespace {
[[maybe_unused]] constexpr const char* alEnumToString(ALenum error) {
    switch (error) {
        case AL_NO_ERROR:
            return "AL_NO_ERROR";
        case AL_INVALID_NAME:
            return "AL_INVALID_NAME";
        case AL_INVALID_ENUM:
            return "AL_INVALID_ENUM";
        case AL_INVALID_VALUE:
            return "AL_INVALID_VALUE";
        case AL_INVALID_OPERATION:
            return "AL_INVALID_OPERATION";
        case AL_OUT_OF_MEMORY:
            return "AL_OUT_OF_MEMORY";
        default:
            return "UNKNOWN_AL_ERROR";
    }
}

[[maybe_unused]] constexpr const char* alcEnumToString(ALCenum error) {
    switch (error) {
        case ALC_NO_ERROR:
            return "ALC_NO_ERROR";
        case ALC_INVALID_DEVICE:
            return "ALC_INVALID_DEVICE";
        case ALC_INVALID_CONTEXT:
            return "ALC_INVALID_CONTEXT";
        case ALC_INVALID_ENUM:
            return "ALC_INVALID_ENUM";
        case ALC_INVALID_VALUE:
            return "ALC_INVALID_VALUE";
        case ALC_OUT_OF_MEMORY:
            return "ALC_OUT_OF_MEMORY";
        default:
            return "UNKNOWN_ALC_ERROR";
    }
}

[[maybe_unused]] [[nodiscard]] bool checkALError(std::source_location loc = std::source_location::current()) {
    if (ALenum error = alGetError(); error != AL_NO_ERROR) {
        MLE_E("OpenAL Error: {} at {}:{}", alEnumToString(error), loc.file_name(), loc.line());
        return false;
    }
    return true;
}

[[maybe_unused]] [[nodiscard]] bool checkALCError(ALCdevice* device, std::source_location loc = std::source_location::current()) {
    if (ALCenum error = alcGetError(device); error != ALC_NO_ERROR) {
        MLE_E("OpenALC Error: {} at {}:{}", alcEnumToString(error), loc.file_name(), loc.line());
        return false;
    }
    return true;
}

/// Non-void OpenAL calls → Expected<T>
template <typename F, typename... Args>
    requires(!std::is_void_v<std::invoke_result_t<F&, Args...>>)
[[nodiscard]] Expected<std::invoke_result_t<F&, Args...>> alCallImpl(std::source_location loc, F&& f, Args&&... args) {
    using Ret = std::invoke_result_t<F&, Args...>;
    Ret ret = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    if (checkALError(loc)) {
        return ret;
    }
    return std::unexpected(Result::OAL_ERROR);
}

template <typename F, typename... Args>
    requires(std::is_void_v<std::invoke_result_t<F&, Args...>>)
[[nodiscard]] bool alCallImpl(std::source_location loc, F&& f, Args&&... args) {
    std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    return checkALError(loc);
}

template <typename F, typename... Args>
    requires(!std::is_void_v<std::invoke_result_t<F&, Args...>>)
[[nodiscard]] Expected<std::invoke_result_t<F&, Args...>> alcCallImpl(std::source_location loc, ALCdevice* device, F&& f, Args&&... args) {
    using Ret = std::invoke_result_t<F&, Args...>;
    Ret ret = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    if (checkALCError(device, loc)) {
        return ret;
    }
    return std::unexpected(Result::OAL_ERROR);
}

template <typename F, typename... Args>
    requires(std::is_void_v<std::invoke_result_t<F&, Args...>>)
[[nodiscard]] bool alcCallImpl(std::source_location loc, ALCdevice* device, F&& f, Args&&... args) {
    std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    return checkALCError(device, loc);
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define alCall(...) mle::alCallImpl(std::source_location::current(), __VA_ARGS__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define alcCall(...) mle::alcCallImpl(std::source_location::current(), device_, __VA_ARGS__)
}  // namespace

Result AudioEngine::init() {
    MLE_I("Initializing Audio Engine");

    device_ = alcOpenDevice(nullptr);
    if (!device_) {
        MLE_E("Failed to open OpenAL device");
        return Result::OAL_ERROR;
    }

    if (auto context_r = alcCall(alcCreateContext, device_, nullptr); context_r) {
        context_ = context_r.value();
    } else {
        MLE_C("Failed to create OpenAL context: {}", context_r.error());
        alcCloseDevice(device_);
        return context_r.error();
    }

    logAvailableDevices();

    run_thread_ = std::jthread([this](std::stop_token st) { runLoop(std::move(st)); });

    return Result::OK;
}

void AudioEngine::logAvailableDevices() {
    if constexpr (!IS_DEBUG_BUILD) {
        return;
    }
    MLE_I("Available Audio Devices:");
    auto devices_r = getAvailableDevices();
    if (!devices_r) {
        MLE_E("Failed to get available audio devices: {}", toString(devices_r.error()));
        return;
    }

    for (const auto& device : devices_r.value()) {
        MLE_I(" - {}", device);
    }
}

void AudioEngine::shutdown() {
    MLE_I("Shutting down Audio Engine");
    if (run_thread_.joinable()) {
        run_thread_.request_stop();
        run_thread_.join();
    }

    if (context_) {
        std::ignore = alcCall(alcDestroyContext, context_);
        context_ = nullptr;
    }

    if (device_) {
        alcCloseDevice(device_);
        device_ = nullptr;
    }
}

Expected<std::vector<std::string>> AudioEngine::getAvailableDevices() {
    auto c = alcCall(alcGetString, device_, ALC_DEVICE_SPECIFIER);
    if (!c) {
        return std::unexpected(c.error());
    }

    std::vector<std::string> ret;
    const char* ptr = c.value();

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) C buffer
    for (; ptr && *ptr != '\0'; ptr += std::strlen(ptr) + 1) {
        ret.emplace_back(ptr);
    }

    return ret;
}

void AudioEngine::stopStream(u8 id) {
    if (id >= streaming_sources_.size()) {
        MLE_E("Invalid streaming source id: {}", id);
        return;
    }

    auto& stream = streaming_sources_.at(id);
    if (stream.active && stream.source != 0) {
        std::ignore = alCall(alSourceStop, stream.source);
        std::array<ALuint, Streaming::BUFFER_COUNT> unqueued{};
        std::ignore = alCall(alSourceUnqueueBuffers, stream.source, stream.queued_buffer_count, unqueued.data());
        stream.current_buffer = 0;
        stream.wav = {};
        stream.looping = false;
        stream.bus = 0;
        stream.active = false;
        stream.queued_buffer_count = 0;
    }
}

void AudioEngine::freeAllSources() {
    MLE_D("Freeing all OpenAL sources.");

    for (usize i = 0; i < streaming_sources_.size(); i++) {
        auto& ss = streaming_sources_.at(i);
        if (ss.source != 0) {
            stopStream(i);
            for (usize j = 0; j < Streaming::BUFFER_COUNT; ++j) {
                if (ss.buffers.at(j) != 0) {
                    MLE_T("Deleted streaming buffer ID: {}", ss.buffers.at(j));
                    std::ignore = alCall(alDeleteBuffers, 1, &ss.buffers.at(j));
                    ss.buffers.at(j) = 0;
                }
            }
            MLE_T("Deleted streaming source ID: {}", ss.source);
            std::ignore = alCall(alDeleteSources, 1, &ss.source);
            ss.active = false;
        }
    }
    streaming_sources_.fill({});

    for (auto& source : one_shot_sources_) {
        MLE_T("Deleted one-shot source ID: {}", source.source);
        std::ignore = alCall(alDeleteSources, 1, &source.source);
    }
    one_shot_sources_.clear();
}

Result AudioEngine::genSources(usize target) {
    MLE_D("Generating OpenAL sources. target: {}", target);

    freeAllSources();

    MLE_D("Streaming sources will use {} sources.", MAX_STREAMING_SOURCES);
    std::array<ALuint, MAX_STREAMING_SOURCES> streaming_source_ids{};
    if (!alcCall(alGenSources, MAX_STREAMING_SOURCES, streaming_source_ids.data())) {
        MLE_E("Failed to generate streaming sources.");
        return Result::OAL_ERROR;
    }
    for (usize i = 0; i < MAX_STREAMING_SOURCES; i++) {
        streaming_sources_.at(i).source = streaming_source_ids.at(i);
        std::ignore = alcCall(alGenBuffers, Streaming::BUFFER_COUNT, streaming_sources_.at(i).buffers.data());
        MLE_T("Generated streaming source ID: {}", streaming_source_ids.at(i));
    }

    usize max_os_sources = target == max<usize>() ? 256 : target;
    max_os_sources -= AudioEngine::MAX_STREAMING_SOURCES;

    one_shot_sources_.reserve(max_os_sources);
    ALuint test_source = 0;
    ALuint last_source = 888;

    MLE_D("Generating up to {} one-shot sources.", max_os_sources);
    for (usize i = 0; i < max_os_sources; i++) {
        std::ignore = alcCall(alGenSources, 1, &test_source);
        if (!test_source || test_source == last_source) {
            MLE_D("Generated {} one-shot sources.", one_shot_sources_.size());
            break;
        }
        last_source = test_source;
        one_shot_sources_.push_back({.source = test_source});
        MLE_T("Generated one-shot source ID: {}", test_source);
    }

    one_shot_sources_.shrink_to_fit();

    return Result::OK;
}

void AudioEngine::updateSources() {
    for (auto& source : one_shot_sources_) {
        if (source.priority > 0) {
            ALint state = AL_STOPPED;
            auto r = alCall(alGetSourcei, source.source, AL_SOURCE_STATE, &state);
            if (!r) {
                MLE_E("Failed to get source state for source ID: {}", source.source);
                continue;
            }
            if (state == AL_STOPPED) {
                source.priority = 0;
            }
        }
    }

    for (usize sid = 0; sid < streaming_sources_.size(); sid++) {
        auto& stream = streaming_sources_.at(sid);
        if (!stream.active || stream.source == 0) {
            continue;
        }

        ALint processed = 0;
        std::ignore = alCall(alGetSourcei, stream.source, AL_BUFFERS_PROCESSED, &processed);

        for (ALint i = 0; i < processed; ++i) {
            ALuint buf = 0;
            std::ignore = alCall(alSourceUnqueueBuffers, stream.source, 1, &buf);
            stream.queued_buffer_count--;
            fillStreamingBuffer(stream);
        }

        int active = AL_STOPPED;
        std::ignore = alCall(alGetSourcei, stream.source, AL_SOURCE_STATE, &active);
        if (active == AL_STOPPED) {
            stopStream(as<u8>(sid));
        }
    }
}

// NOLINTNEXTLINE(performance-unnecessary-value-param) stop_token is small and cheap to copy
void AudioEngine::runLoop(std::stop_token st) {
    if (const auto r = alcCall(alcMakeContextCurrent, context_); !r.has_value()) {
        MLE_C("Failed to make OpenAL context current: {}", r.error());
        return;
    }

    if (!alIsExtensionPresent("AL_EXT_FLOAT32")) {
        // TODO: fallbacks
        MLE_C("OpenAL device does not support AL_EXT_FLOAT32 extension. Falling back to 16-bit audio.");
        return;
    }

    if (isError(genSources())) {
        MLE_C("Failed to generate OpenAL sources during AudioEngine run loop initialization.");
        return;
    }

    while (!st.stop_requested()) {
        {
            MLE_PERF_SCOPE("AudioEngine");

            {
                MLE_PERF_SCOPE("AudioEngine.CmdProcessing");
                processCmds();
            }

            updateSources();
        }
        std::this_thread::sleep_for(10ms);
    }

    freeAllSources();

    std::ignore = alcCall(alcMakeContextCurrent, nullptr);
}

[[nodiscard]] f32 VolumeMixer::compute(u8 b, f32 source_linear) const {
    f32 bus = 1.0F;
    if (b < bus_volumes_db_.size()) {
        bus = dbToLinear(bus_volumes_db_.at(b));
    }
    return std::clamp(master_ * bus * source_linear, 0.0F, 4.0F);
}

void AudioEngine::enqueueCmd(const audio::Cmd& cmd) {
    cmd_queue_.push(cmd);
};

void AudioEngine::processCmds() {
    auto cmds = cmd_queue_.popAll();
    for (const auto& cmd : cmds) {
        processCmd(cmd);
    }
}

void AudioEngine::processCmd(const audio::Cmd& cmd) {
    std::visit(Overloaded{
                   [&](const audio::cmd::Load& l) { processCmdLoad(l); },
                   [&](const audio::cmd::PlayOneShot& p) { processCmdPlayOneShot(p); },
                   [&](const audio::cmd::StartStream& p) { processCmdStartStream(p); },
                   [&](const audio::cmd::StopStream& s) { processCmdStop(s); },
                   [&](const audio::cmd::PauseStream& p) { processCmdPause(p); },
                   [&](const audio::cmd::ResumeStream& r) { processCmdResume(r); },
                   [&](const audio::cmd::SetVolume& sv) { processCmdSetVolume(sv); },
                   [&](const audio::cmd::SetListener& sl) { processCmdSetListener(sl); },
                   [&](const audio::cmd::SetDistanceParams& sdp) { processCmdSetDistanceParams(sdp); },
                   [&](const audio::cmd::StopAll& sa) { processCmdStopAll(sa); },
               },
               cmd);
}

void AudioEngine::processCmdLoad(const audio::cmd::Load& cmd) {
    Path path = ResPath::RES;
    path /= ResPath::SOUNDS;
    path /= cmd.name;
    path += ".wav";

    entt::id_type sound_id = entt::hashed_string::value(cmd.name.c_str());

    if (cmd.stream) {
        if (stream_sounds_.contains(sound_id)) {
            MLE_W("Stream sound already loaded: {} (ID: {}), replacing.", cmd.name, sound_id);
        }
        stream_sounds_[sound_id] = path;
        return;
    }

    Expected<WavData> wav_data = loadWavFile(path);
    if (!wav_data) {
        MLE_E("Failed to load WAV file: {}: {}", path, toString(wav_data.error()));
        return;
    }
    auto& wav = wav_data.value();

    ALenum format = AL_NONE;
    if (wav.channels == 1) {
        format = AL_FORMAT_MONO_FLOAT32;
    } else if (wav.channels == 2) {
        format = AL_FORMAT_STEREO_FLOAT32;
    } else {
        MLE_E("Unsupported channel count on wav: {} file: {}", wav.channels, path);
        return;
    }

    ALuint buffer = 0;
    auto call_r = alCall(alGenBuffers, 1, &buffer);
    if (!call_r) {
        MLE_E("Failed to generate OpenAL buffer for wav: {}", path);
        return;
    }
    call_r = alCall(alBufferData, buffer, format, wav.samples.data(), as<ALsizei>(wav.samples.size() * sizeof(f32)), wav.sample_rate);
    if (!call_r) {
        MLE_E("Failed to upload WAV data to OpenAL buffer for wav: {}", path);
        std::ignore = alCall(alDeleteBuffers, 1, &buffer);
        return;
    }

    if (loaded_sounds_.contains(sound_id)) {
        MLE_W("Sound already loaded: {} (ID: {}), replacing.", cmd.name, sound_id);
        ALuint old_buffer = loaded_sounds_.at(sound_id);
        std::ignore = alCall(alDeleteBuffers, 1, &old_buffer);
    }
    loaded_sounds_.emplace(sound_id, buffer);
    MLE_D("Loaded sound: {} (ID: {}), size: {} bytes, channels: {}, sample rate: {}", cmd.name, sound_id, wav.samples.size() * sizeof(f32), wav.channels,
          wav.sample_rate);
};

void AudioEngine::processCmdPlayOneShot(const audio::cmd::PlayOneShot& cmd) {
    auto buffer_it = loaded_sounds_.find(cmd.sound_id);
    if (buffer_it == loaded_sounds_.end()) {
        MLE_W("Sound ID: {} not loaded, cannot play.", cmd.sound_id);
        return;
    }
    if (cmd.priority == 0) {
        MLE_W("Sound with 0 priority cannot be played. Sound ID: {}", cmd.sound_id);
        return;
    }

    ALuint source = 0;
    auto lowest_priority = max<usize>();
    auto lowest_priority_idx = max<usize>();
    for (usize i = 0; i < one_shot_sources_.size(); i++) {
        auto& s = one_shot_sources_[i];
        if (s.priority == 0) {
            source = s.source;
            s.priority = cmd.priority;
            break;
        }
        if (s.priority < lowest_priority) {
            lowest_priority = s.priority;
            lowest_priority_idx = i;
        }
    }
    if (source == 0) {
        if (cmd.priority > lowest_priority) {
            auto& s = one_shot_sources_[lowest_priority_idx];
            source = s.source;
            s.priority = cmd.priority;
            std::ignore = alCall(alSourceStop, source);
        } else {
            MLE_T("No available audio sources to play sound ID: {} with priority: {}", cmd.sound_id, cmd.priority);
            return;
        }
    }

    std::ignore = alCall(alSourcef, source, AL_PITCH, 1);
    std::ignore = alCall(alSource3f, source, AL_POSITION, 0, 0, 0);
    std::ignore = alCall(alSource3f, source, AL_VELOCITY, 0, 0, 0);
    std::ignore = alCall(alSourcef, source, AL_GAIN, 1.0F);
    std::ignore = alCall(alSourcei, source, AL_LOOPING, AL_FALSE);
    std::ignore = alCall(alSourcei, source, AL_BUFFER, buffer_it->second);

    std::ignore = alCall(alSourcePlay, source);
};

void AudioEngine::processCmdStartStream(const audio::cmd::StartStream& cmd) {
    auto path_it = stream_sounds_.find(cmd.sound_id);
    if (path_it == stream_sounds_.end()) {
        MLE_W("Stream sound ID: {} not loaded, cannot start stream.", cmd.sound_id);
        return;
    }
    auto& path = path_it->second;

    if (cmd.id >= streaming_sources_.size()) {
        MLE_E("Invalid streaming source id: {}", cmd.id);
        return;
    }

    stopStream(cmd.id);

    auto& stream = streaming_sources_.at(cmd.id);

    Expected<WavData> wav_data = loadWavFile(path);
    if (!wav_data) {
        MLE_E("Failed to load WAV file: {}: {}", path, toString(wav_data.error()));
        return;
    }

    stream.wav = std::move(wav_data.value());

    const usize total_samples = stream.wav.samples.size();
    usize offset = 0;

    if (cmd.params.start_offset_ms > 0) {
        offset = (stream.wav.sample_rate * stream.wav.channels * cmd.params.start_offset_ms) / 1000;
        if (offset > total_samples) {
            MLE_E("Start offset {}ms exceeds total duration of sound ID: {}", cmd.params.start_offset_ms, cmd.sound_id);
            return;
        }
    }

    usize duration_samples = total_samples - offset;
    if (cmd.params.duration_ms > 0) {
        usize ndur_samples = (stream.wav.sample_rate * stream.wav.channels * cmd.params.duration_ms) / 1000;
        if (ndur_samples == 0) {
            MLE_E("Duration {}ms is too short for sound ID: {}", cmd.params.duration_ms, cmd.sound_id);
            return;
        }
        if (offset + ndur_samples > total_samples) {
            MLE_W("Duration {}ms exceeds total duration of sound ID: {} with offset {}ms, adjusting. Leave it as 0 if you want remaining duration.",
                  cmd.params.duration_ms, cmd.sound_id, cmd.params.start_offset_ms);
            ndur_samples = total_samples - offset;
        }
    }

    if (total_samples <= Streaming::SAMPLES_PER_BUFFER) {
        MLE_E("Sound ID: {} is too short to stream with offset_ms: {}, duration_ms: {}. {} samples.", cmd.sound_id, cmd.params.start_offset_ms,
              cmd.params.duration_ms, total_samples);
        return;
    }

    stream.first_sample = offset;
    stream.current_sample = stream.first_sample;
    stream.last_sample = stream.first_sample + duration_samples;
    stream.current_buffer = 0;
    stream.queued_buffer_count = 0;
    stream.looping = cmd.loop;
    stream.bus = cmd.params.bus;
    stream.active = true;

    std::ignore = alCall(alSourcei, stream.source, AL_BUFFER, 0);

    for (usize i = 0; i < Streaming::BUFFER_COUNT; ++i) {
        fillStreamingBuffer(stream);
    }

    std::ignore = alCall(alSourcef, stream.source, AL_PITCH, cmd.params.pitch);
    std::ignore = alCall(alSource3f, stream.source, AL_POSITION, cmd.params.position.x, cmd.params.position.y, cmd.params.position.z);
    std::ignore = alCall(alSource3f, stream.source, AL_VELOCITY, cmd.params.velocity.x, cmd.params.velocity.y, cmd.params.velocity.z);
    std::ignore = alCall(alSourcef, stream.source, AL_GAIN, cmd.params.volume);
    std::ignore = alCall(alSourcei, stream.source, AL_LOOPING, AL_FALSE);

    std::ignore = alCall(alSourcePlay, stream.source);
};

void AudioEngine::fillStreamingBuffer(Streaming& stream) {
    usize remain = stream.last_sample - stream.current_sample;
    ALenum format = (stream.wav.channels == 1) ? AL_FORMAT_MONO_FLOAT32 : AL_FORMAT_STEREO_FLOAT32;

    auto buf = stream.buffers.at(stream.current_buffer);

    MLE_ASSERT(stream.active);

    if (stream.looping) {
        if (remain >= Streaming::SAMPLES_PER_BUFFER) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) C buffer
            std::ignore = alCall(alBufferData, buf, format, stream.wav.samples.data() + stream.current_sample, Streaming::SAMPLES_PER_BUFFER * sizeof(f32),
                                 stream.wav.sample_rate);
            stream.current_sample += Streaming::SAMPLES_PER_BUFFER;
        } else {
            std::array<f32, Streaming::SAMPLES_PER_BUFFER> temp_buffer{};
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) C buffer
            std::memcpy(temp_buffer.data(), stream.wav.samples.data() + stream.current_sample, remain * sizeof(f32));
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) C buffer
            std::memcpy(temp_buffer.data() + remain, stream.wav.samples.data() + stream.first_sample, (Streaming::SAMPLES_PER_BUFFER - remain) * sizeof(f32));
            std::ignore = alCall(alBufferData, buf, format, temp_buffer.data(), Streaming::SAMPLES_PER_BUFFER * sizeof(f32), stream.wav.sample_rate);
            stream.current_sample = stream.first_sample + (Streaming::SAMPLES_PER_BUFFER - remain);
        }
    } else {
        usize to_copy = std::min(Streaming::SAMPLES_PER_BUFFER, remain);
        if (to_copy == 0) {
            return;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) C buffer
        std::ignore = alCall(alBufferData, buf, format, stream.wav.samples.data() + stream.current_sample, to_copy * sizeof(f32), stream.wav.sample_rate);
        stream.current_sample += to_copy;
    }

    std::ignore = alCall(alSourceQueueBuffers, stream.source, 1, &buf);
    stream.current_buffer = (stream.current_buffer + 1) % Streaming::BUFFER_COUNT;
    stream.queued_buffer_count++;
};

void AudioEngine::processCmdStop(const audio::cmd::StopStream& cmd) {
    std::ignore = cmd;
};

void AudioEngine::processCmdPause(const audio::cmd::PauseStream& cmd) {
    std::ignore = cmd;
};

void AudioEngine::processCmdResume(const audio::cmd::ResumeStream& cmd) {
    std::ignore = cmd;
};

void AudioEngine::processCmdSetVolume(const audio::cmd::SetVolume& cmd) {
    std::ignore = cmd;
};

void AudioEngine::processCmdSetListener(const audio::cmd::SetListener& cmd) {
    std::ignore = cmd;
};

void AudioEngine::processCmdSetDistanceParams(const audio::cmd::SetDistanceParams& cmd) {
    std::ignore = cmd;
};

void AudioEngine::processCmdStopAll(const audio::cmd::StopAll& cmd) {
    std::ignore = cmd;
};
}  // namespace mle
