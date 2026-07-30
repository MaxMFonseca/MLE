#include "AudioEngine.h"

#include <AL/al.h>
#include <AL/alext.h>

#include <algorithm>
#include <cmath>
#include <expected>
#include <sol/forward.hpp>
#include <source_location>

#include "mle/audio/StreamState.h"
#include "mle/audio/Types.h"
#include "mle/audio/Utils.h"
#include "mle/client/Client.h"
#include "mle/core/Assert.h"
#include "mle/core/Consts.h"
#include "mle/core/Logger.h"
#include "mle/core/PerfTracker.h"
#include "mle/lua/Utils.h"
#include "mle/math/Types.h"
#include "mle/utils/String.h"

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

bool checkedSourcePlayv(ALsizei count, const ALuint* sources) {
    return alCall(alSourcePlayv, count, sources);
}
}  // namespace

Result AudioEngine::init() {
    MLE_I("Initializing Audio Engine");

    cmd_mailbox_.reset();
    thread_startup_.reset();
    initRTCLs();

    bus_volumes_.fill(1.0F);
    bus_voice_policies_ = {};

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

    const auto startup_result = thread_startup_.wait();
    if (startup_result != Result::OK) {
        run_thread_.join();
        std::ignore = alcCall(alcDestroyContext, context_);
        context_ = nullptr;
        alcCloseDevice(device_);
        device_ = nullptr;
    }
    return startup_result;
}

void AudioEngine::initRTCLs() {
    MLE_D("Initializing AudioEngine RTCLs");

    // FIXME: This is ugly but its only for debug purposes now
    // I need to create a better command line parser in order to make this usable

    rtcls_.at(as<usize>(RTCLs::PLAY_ONE_SHOT))
        .setKey("audio.play_one_shot")
        .setCallback([this](const std::string& v) {
            auto ss = split(v, ' ');
            if (ss.size() != 1) {
                MLE_E("audio.play_one_shot requires 1 argument: <sound_name>");  // NOLINT
                return false;
            }
            audio::cmd::PlayOneShot cmd;
            const std::string sound_name{ss.at(0)};
            cmd.sound_id = entt::hashed_string{sound_name.c_str()};
            enqueueCmd(cmd);
            return false;
        })
        .listen();

    rtcls_.at(as<usize>(RTCLs::START_STREAM))
        .setKey("audio.start_stream")
        .setCallback([this](const std::string& v) {
            auto ss = split(v, ' ');
            if (ss.size() != 3) {
                MLE_E("audio.start_stream requires 2 arguments: <sound_name> <id> <bus>");  // NOLINT
                return false;
            }
            audio::cmd::StartStream cmd;
            const std::string sound_name{ss.at(0)};
            cmd.sound_id = entt::hashed_string{sound_name.c_str()};
            cmd.id = strTo<u8>(ss.at(1)).value_or(0);
            cmd.params.bus = strTo<u8>(ss.at(2)).value_or(0);
            enqueueCmd(cmd);
            return false;
        })
        .listen();

    rtcls_.at(as<usize>(RTCLs::STOP_STREAM))
        .setKey("audio.stop_stream")
        .setCallback([this](const std::string& v) {
            auto ss = split(v, ' ');
            if (ss.size() != 1) {
                MLE_E("audio.stop_stream requires 1 argument: <stream_id>");  // NOLINT
                return false;
            }
            audio::cmd::StopStream cmd;
            cmd.id = strTo<u8>(ss.at(0)).value_or(0);
            enqueueCmd(cmd);
            return false;
        })
        .listen();

    rtcls_.at(as<usize>(RTCLs::PAUSE_STREAM))
        .setKey("audio.pause_stream")
        .setCallback([this](const std::string& v) {
            auto ss = split(v, ' ');
            if (ss.size() != 1) {
                MLE_E("audio.pause_stream requires 1 argument: <stream_id>");  // NOLINT
                return false;
            }
            audio::cmd::PauseStream cmd;
            cmd.id = strTo<u8>(ss.at(0)).value_or(0);
            enqueueCmd(cmd);
            return false;
        })
        .listen();

    rtcls_.at(as<usize>(RTCLs::RESUME_STREAM))
        .setKey("audio.resume_stream")
        .setCallback([this](const std::string& v) {
            auto ss = split(v, ' ');
            if (ss.size() != 1) {
                MLE_E("audio.resume_stream requires 1 argument: <stream_id>");  // NOLINT
                return false;
            }
            audio::cmd::ResumeStream cmd;
            cmd.id = strTo<u8>(ss.at(0)).value_or(0);
            enqueueCmd(cmd);
            return false;
        })
        .listen();

    rtcls_.at(as<usize>(RTCLs::SET_VOLUME))
        .setKey("audio.set_volume")
        .setCallback([this](const std::string& v) {
            auto ss = split(v, ' ');
            if (ss.size() != 2) {
                MLE_E("audio.set_volume requires 2 arguments: <bus> <volume>");  // NOLINT
                return false;
            }
            audio::cmd::SetVolume cmd;
            cmd.bus = strTo<u8>(ss.at(0)).value_or(0);
            cmd.volume = strTo<f32>(ss.at(1)).value_or(1.0F);
            enqueueCmd(cmd);
            return false;
        })
        .listen();

    rtcls_.at(as<usize>(RTCLs::STOP_ALL))
        .setKey("audio.stop_all")
        .setCallback([this](const std::string&) {
            audio::cmd::StopAll cmd;
            enqueueCmd(cmd);
            return false;
        })
        .listen();
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
    cmd_mailbox_.close();
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

Result AudioEngine::stopStream(u8 id) {
    if (id >= streaming_sources_.size()) {
        MLE_E("Invalid streaming source id: {}", id);
        return Result::OUT_OR_RANGE;
    }

    auto& stream = streaming_sources_.at(id);
    if ((stream.active || stream.cleanup_pending) && stream.source != 0) {
        stream.active = false;
        stream.cleanup_pending = true;
        if (!alCall(alSourceStop, stream.source)) {
            return Result::OAL_ERROR;
        }
        std::array<ALuint, Streaming::BUFFER_COUNT> unqueued{};
        if (!alCall(alSourceUnqueueBuffers, stream.source, stream.queued_buffer_count, unqueued.data())) {
            return Result::OAL_ERROR;
        }
    }
    stream.current_buffer = 0;
    stream.current_sample = 0;
    stream.first_sample = 0;
    stream.last_sample = 0;
    stream.wav.reset();
    stream.looping = false;
    stream.bus = 0;
    stream.volume = 1.0F;
    stream.ramp = {};
    stream.ramp_tick = {};
    stream.active = false;
    stream.cleanup_pending = false;
    stream.paused = false;
    stream.queued_buffer_count = 0;
    return Result::OK;
}

void AudioEngine::freeAllSources() {
    MLE_D("Freeing all OpenAL sources.");

    for (usize i = 0; i < streaming_sources_.size(); i++) {
        auto& ss = streaming_sources_.at(i);
        if (ss.source != 0) {
            std::ignore = stopStream(as<u8>(i));
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

    for (usize i = 0; i < one_shot_sources_.size(); ++i) {
        auto& source = one_shot_sources_[i];
        audio::resetVoice(one_shot_voice_metadata_[i]);
        MLE_T("Deleted one-shot source ID: {}", source.source);
        std::ignore = alCall(alDeleteSources, 1, &source.source);
    }
    one_shot_sources_.clear();
    one_shot_voice_metadata_.clear();
}

Result AudioEngine::genSources(usize target) {
    MLE_D("Generating OpenAL sources. target: {}", target);

    freeAllSources();

    MLE_D("Streaming sources will use {} sources.", audio::STREAM_SLOT_COUNT);
    std::array<ALuint, audio::STREAM_SLOT_COUNT> streaming_source_ids{};
    if (!alcCall(alGenSources, audio::STREAM_SLOT_COUNT, streaming_source_ids.data())) {
        MLE_E("Failed to generate streaming sources.");
        return Result::OAL_ERROR;
    }
    for (usize i = 0; i < audio::STREAM_SLOT_COUNT; i++) {
        streaming_sources_.at(i).source = streaming_source_ids.at(i);
        std::ignore = alcCall(alGenBuffers, Streaming::BUFFER_COUNT, streaming_sources_.at(i).buffers.data());
        MLE_T("Generated streaming source ID: {}", streaming_source_ids.at(i));
    }

    usize max_os_sources = target == max<usize>() ? 256 : target;
    max_os_sources -= audio::STREAM_SLOT_COUNT;

    one_shot_sources_.reserve(max_os_sources);
    one_shot_voice_metadata_.reserve(max_os_sources);
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
        one_shot_voice_metadata_.emplace_back();
        MLE_T("Generated one-shot source ID: {}", test_source);
    }

    one_shot_sources_.shrink_to_fit();
    one_shot_voice_metadata_.shrink_to_fit();

    return Result::OK;
}

void AudioEngine::updateSources() {
    for (usize i = 0; i < one_shot_sources_.size(); ++i) {
        auto& source = one_shot_sources_[i];
        auto& metadata = one_shot_voice_metadata_[i];
        if (metadata.priority > 0) {
            ALint state = AL_STOPPED;
            auto r = alCall(alGetSourcei, source.source, AL_SOURCE_STATE, &state);
            if (!r) {
                MLE_E("Failed to get source state for source ID: {}", source.source);
                continue;
            }
            if (state == AL_STOPPED) {
                audio::resetVoice(metadata);
            }
        }
    }

    for (usize sid = 0; sid < streaming_sources_.size(); sid++) {
        auto& stream = streaming_sources_.at(sid);
        if (stream.cleanup_pending) {
            std::ignore = stopStream(as<u8>(sid));
            continue;
        }
        if (!stream.active || stream.source == 0) {
            continue;
        }

        if (stream.ramp.active && !stream.paused) {
            const auto now = std::chrono::steady_clock::now();
            const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - stream.ramp_tick);
            stream.ramp_tick = now;
            const auto advanced = audio::advanceRamp(stream.ramp, delta);
            stream.volume = advanced.volume;
            std::ignore = applyVolume(stream.source, stream.bus, stream.volume);
            if (advanced.completion == audio::RampCompletion::STOP) {
                std::ignore = stopStream(as<u8>(sid));
                continue;
            }
        }

        ALint processed = 0;
        std::ignore = alCall(alGetSourcei, stream.source, AL_BUFFERS_PROCESSED, &processed);

        for (ALint i = 0; i < processed; ++i) {
            ALuint buf = 0;
            std::ignore = alCall(alSourceUnqueueBuffers, stream.source, 1, &buf);
            stream.queued_buffer_count--;
            std::ignore = fillStreamingBuffer(stream);
        }

        int active = AL_STOPPED;
        std::ignore = alCall(alGetSourcei, stream.source, AL_SOURCE_STATE, &active);
        if (active == AL_STOPPED) {
            std::ignore = stopStream(as<u8>(sid));
        }
    }
}

// NOLINTNEXTLINE(performance-unnecessary-value-param) stop_token is small and cheap to copy
void AudioEngine::runLoop(std::stop_token st) {
    if (const auto r = alcCall(alcMakeContextCurrent, context_); !r.has_value()) {
        MLE_C("Failed to make OpenAL context current: {}", r.error());
        thread_startup_.publish(r.error());
        return;
    }

    if (!alIsExtensionPresent("AL_EXT_FLOAT32")) {
        // TODO: fallbacks
        MLE_C("OpenAL device does not support AL_EXT_FLOAT32 extension. Falling back to 16-bit audio.");
        std::ignore = alcCall(alcMakeContextCurrent, nullptr);
        thread_startup_.publish(Result::OAL_ERROR);
        return;
    }

    if (const auto source_result = genSources(); isError(source_result)) {
        MLE_C("Failed to generate OpenAL sources during AudioEngine run loop initialization.");
        freeAllSources();
        std::ignore = alcCall(alcMakeContextCurrent, nullptr);
        thread_startup_.publish(source_result);
        return;
    }

    if (st.stop_requested()) {
        freeAllSources();
        std::ignore = alcCall(alcMakeContextCurrent, nullptr);
        thread_startup_.publish(Result::NOT_READY);
        return;
    }

    cmd_mailbox_.open();
    thread_startup_.publish(Result::OK);

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

    audio::drainBeforeTeardown(cmd_mailbox_, [this](const audio::Cmd& cmd) { processCmd(cmd); }, [this] { freeAllSources(); });

    std::ignore = alcCall(alcMakeContextCurrent, nullptr);
}

void AudioEngine::setBusVolumeLinear(u8 b, f32 linear) {
    if (b >= bus_volumes_.size()) {
        MLE_W("Invalid bus index: {}", b);
        return;
    }
    bus_volumes_.at(b) = std::clamp(linear, 0.0F, 4.0F);
    MLE_D("Bus {} volume set to {}", b, bus_volumes_.at(b));
}

Result AudioEngine::applyVolume(ALuint source, u8 bus, f32 source_linear) const {
    if (bus >= bus_volumes_.size()) {
        return Result::OUT_OR_RANGE;
    }

    const f32 master = bus_volumes_.at(0);
    const f32 bus_volume = bus == 0 ? 1.0F : bus_volumes_.at(bus);
    const f32 gain = std::clamp(master * bus_volume * source_linear, 0.0F, 4.0F);
    return alCall(alSourcef, source, AL_GAIN, gain) ? Result::OK : Result::OAL_ERROR;
}

Result AudioEngine::applySourcePlaybackState(ALuint source, const audio::SourcePlaybackState& state) {
    bool success = true;
    success &= alCall(alSourcei, source, AL_SOURCE_RELATIVE, state.relative ? AL_TRUE : AL_FALSE);
    success &= alCall(alSource3f, source, AL_POSITION, state.position.x, state.position.y, state.position.z);
    success &= alCall(alSource3f, source, AL_VELOCITY, state.velocity.x, state.velocity.y, state.velocity.z);
    return success ? Result::OK : Result::OAL_ERROR;
}

void AudioEngine::enqueueCmd(const audio::Cmd& cmd) {
    switch (cmd_mailbox_.tryPush(cmd)) {
        case audio::CommandSubmitResult::ACCEPTED:
            return;
        case audio::CommandSubmitResult::FULL:
            MLE_W("Audio command queue is full, dropping command.");
            return;
        case audio::CommandSubmitResult::CLOSED:
            MLE_W("Audio command rejected because AudioEngine is shutting down.");
            return;
    }
}

void AudioEngine::processCmds() {
    cmd_mailbox_.drain([this](const audio::Cmd& cmd) { processCmd(cmd); });
}

void AudioEngine::processCmd(const audio::Cmd& cmd) {
    std::visit(Overloaded{
                   [&](const audio::cmd::Load& l) { processCmdLoad(l); },
                   [&](const audio::cmd::PlayOneShot& p) { processCmdPlayOneShot(p); },
                   [&](const audio::cmd::StartStream& p) { processCmdStartStream(p); },
                   [&](const audio::cmd::StartStreamGroup& group) { processCmdStartStreamGroup(group); },
                   [&](const audio::cmd::StopStream& s) { processCmdStop(s); },
                   [&](const audio::cmd::SetStreamParams& s) { processCmdSetStreamParams(s); },
                   [&](const audio::cmd::PauseStream& p) { processCmdPause(p); },
                   [&](const audio::cmd::ResumeStream& r) { processCmdResume(r); },
                   [&](const audio::cmd::SetVolume& sv) { processCmdSetVolume(sv); },
                   [&](const audio::cmd::SetListener& sl) { processCmdSetListener(sl); },
                   [&](const audio::cmd::SetDistanceParams& sdp) { processCmdSetDistanceParams(sdp); },
                   [&](const audio::cmd::StopAll& sa) { processCmdStopAll(sa); },
                   [&](const audio::cmd::SetBusVoicePolicy& sbvp) { processCmdSetBusVoicePolicy(sbvp); },
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
        Expected<WavData> wav_data = loadWavFile(path);
        if (!wav_data) {
            MLE_E("Failed to load WAV file: {}: {}", path, toString(wav_data.error()));
            return;
        }
        auto& wav = wav_data.value();
        if (wav.channels != 1 && wav.channels != 2) {
            MLE_E("Unsupported channel count on wav: {} file: {}", wav.channels, path);
            return;
        }
        if (wav.samples.empty()) {
            MLE_E("WAV file contains no sample data: {}", path);
            return;
        }
        if (stream_sounds_.contains(sound_id)) {
            MLE_W("Stream sound already loaded: {} (ID: {}), replacing.", cmd.name, sound_id);
        }
        stream_sounds_.insert_or_assign(sound_id, std::make_shared<const WavData>(std::move(wav)));
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

    const auto selection = audio::selectVoice(one_shot_voice_metadata_, bus_voice_policies_, cmd.params.bus, cmd.priority);
    if (!selection.accepted()) {
        MLE_T("No eligible audio source to play sound ID: {} with priority: {}", cmd.sound_id, cmd.priority);
        return;
    }

    const usize oss_idx = selection.index;
    auto& oss = one_shot_sources_.at(oss_idx);
    auto& metadata = one_shot_voice_metadata_.at(oss_idx);
    const ALuint source = oss.source;
    if (metadata.priority > 0) {
        std::ignore = alCall(alSourceStop, source);
    }
    metadata = {.priority = cmd.priority, .bus = cmd.params.bus, .volume = cmd.params.volume};

    std::ignore = alCall(alSourcei, source, AL_BUFFER, buffer_it->second);
    std::ignore = alCall(alSourcef, source, AL_PITCH, cmd.params.pitch);
    std::ignore = applySourcePlaybackState(source, audio::sourcePlaybackState(cmd.params));
    std::ignore = alCall(alSourcei, source, AL_LOOPING, AL_FALSE);
    std::ignore = applyVolume(oss.source, metadata.bus, metadata.volume);

    std::ignore = alCall(alSourcePlay, source);
};

Expected<AudioEngine::StreamStartPlan> AudioEngine::preflightStreamStart(const audio::cmd::StartStream& cmd) const {
    if (cmd.id >= streaming_sources_.size()) {
        return std::unexpected(Result::OUT_OR_RANGE);
    }
    const auto wav_it = stream_sounds_.find(cmd.sound_id);
    if (wav_it == stream_sounds_.end()) {
        return std::unexpected(Result::NOT_FOUND);
    }
    const auto& wav = wav_it->second;
    auto validated = audio::validateStreamStart(wav->sample_rate, as<u16>(wav->channels), wav->samples.size(), cmd.params.bus, cmd.params.volume,
                                                cmd.params.pitch, cmd.params.start_offset_ms, cmd.params.duration_ms, Streaming::SAMPLES_PER_BUFFER);
    if (!validated) {
        return std::unexpected(validated.error());
    }
    return StreamStartPlan{.cmd = cmd, .wav = wav, .validated = *validated};
}

Result AudioEngine::prepareStream(const StreamStartPlan& plan) {
    const auto stop_result = stopStream(plan.cmd.id);
    if (isError(stop_result)) {
        return stop_result;
    }

    auto& stream = streaming_sources_.at(plan.cmd.id);
    stream.wav = plan.wav;
    stream.first_sample = plan.validated.window.first_sample;
    stream.current_sample = stream.first_sample;
    stream.last_sample = plan.validated.window.last_sample;
    stream.current_buffer = 0;
    stream.queued_buffer_count = 0;
    stream.looping = plan.cmd.loop;
    stream.bus = plan.cmd.params.bus;
    stream.active = true;
    stream.cleanup_pending = false;
    stream.paused = false;
    const f32 target_volume = plan.validated.volume;
    stream.volume = plan.cmd.params.fade_in_ms > 0 ? 0.0F : target_volume;
    stream.ramp = plan.cmd.params.fade_in_ms > 0
                      ? audio::beginRamp(0.0F, target_volume, std::chrono::milliseconds{plan.cmd.params.fade_in_ms}, audio::RampCompletion::NONE)
                      : audio::VolumeRamp{};
    stream.ramp_tick = {};

    if (!alCall(alSourcei, stream.source, AL_BUFFER, 0)) {
        return Result::OAL_ERROR;
    }

    for (usize i = 0; i < Streaming::BUFFER_COUNT; ++i) {
        if (const auto result = fillStreamingBuffer(stream); isError(result)) {
            return result;
        }
    }

    if (!alCall(alSourcef, stream.source, AL_PITCH, plan.validated.pitch)) {
        return Result::OAL_ERROR;
    }
    if (const auto result = applySourcePlaybackState(stream.source, audio::sourcePlaybackState(plan.cmd.params)); isError(result)) {
        return result;
    }
    if (!alCall(alSourcei, stream.source, AL_LOOPING, AL_FALSE)) {
        return Result::OAL_ERROR;
    }
    return applyVolume(stream.source, stream.bus, stream.volume);
}

void AudioEngine::clearStreamTargets(std::span<const u8> slots) {
    for (const u8 slot : slots) {
        std::ignore = stopStream(slot);
    }
}

void AudioEngine::processCmdStartStream(const audio::cmd::StartStream& cmd) {
    auto plan = preflightStreamStart(cmd);
    if (!plan) {
        MLE_W("Cannot start stream sound ID: {} in slot {}: {}", cmd.sound_id, cmd.id, toString(plan.error()));
        return;
    }
    if (const auto result = prepareStream(*plan); isError(result)) {
        MLE_E("Failed to prepare stream sound ID: {} in slot {}: {}", cmd.sound_id, cmd.id, toString(result));
        std::ignore = stopStream(cmd.id);
        return;
    }

    auto& stream = streaming_sources_.at(cmd.id);
    stream.ramp_tick = std::chrono::steady_clock::now();
    if (!alCall(alSourcePlay, stream.source)) {
        MLE_E("Failed to play stream sound ID: {} in slot {}", cmd.sound_id, cmd.id);
        std::ignore = stopStream(cmd.id);
    }
}

void AudioEngine::processCmdStartStreamGroup(const audio::cmd::StartStreamGroup& cmd) {
    const auto structural_result = audio::validateStreamGroup(cmd);
    if (structural_result != audio::StreamGroupRejectReason::NONE) {
        MLE_W("Invalid stream group: {}", audio::streamGroupRejectReasonName(structural_result));
        return;
    }

    std::array<std::optional<StreamStartPlan>, audio::STREAM_SLOT_COUNT> plans{};
    for (usize i = 0; i < cmd.count; ++i) {
        auto plan = preflightStreamStart(cmd.streams.at(i));
        if (!plan) {
            MLE_W("Cannot preflight stream group member {} in slot {}: {}", i, cmd.streams.at(i).id, toString(plan.error()));
            return;
        }
        plans.at(i) = std::move(*plan);
    }

    std::array<u8, audio::STREAM_SLOT_COUNT> target_slots{};
    std::array<ALuint, audio::STREAM_SLOT_COUNT> source_ids{};
    usize touched_count = 0;
    for (usize i = 0; i < cmd.count; ++i) {
        target_slots.at(i) = plans.at(i)->cmd.id;
        ++touched_count;
        if (const auto result = prepareStream(*plans.at(i)); isError(result)) {
            MLE_E("Failed to prepare stream group member {} in slot {}: {}", i, target_slots.at(i), toString(result));
            clearStreamTargets(std::span{target_slots}.first(touched_count));
            return;
        }
        source_ids.at(i) = streaming_sources_.at(target_slots.at(i)).source;
    }

    const auto shared_tick = std::chrono::steady_clock::now();
    for (usize i = 0; i < cmd.count; ++i) {
        streaming_sources_.at(target_slots.at(i)).ramp_tick = shared_tick;
    }

    if (!audio::invokeSynchronizedPlay(std::span{source_ids}.first(cmd.count), checkedSourcePlayv)) {
        MLE_E("Failed to start stream group with {} members", cmd.count);
        clearStreamTargets(std::span{target_slots}.first(cmd.count));
    }
}

Result AudioEngine::fillStreamingBuffer(Streaming& stream) {
    MLE_ASSERT(stream.wav != nullptr);
    const auto& wav = *stream.wav;
    usize remain = stream.last_sample - stream.current_sample;
    ALenum format = (wav.channels == 1) ? AL_FORMAT_MONO_FLOAT32 : AL_FORMAT_STEREO_FLOAT32;

    auto buf = stream.buffers.at(stream.current_buffer);

    MLE_ASSERT(stream.active);

    if (stream.looping) {
        if (remain >= Streaming::SAMPLES_PER_BUFFER) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) C buffer
            if (!alCall(alBufferData, buf, format, wav.samples.data() + stream.current_sample, Streaming::SAMPLES_PER_BUFFER * sizeof(f32), wav.sample_rate)) {
                return Result::OAL_ERROR;
            }
            stream.current_sample = audio::advanceLoopingSampleCursor({.first_sample = stream.first_sample, .last_sample = stream.last_sample},
                                                                      stream.current_sample, Streaming::SAMPLES_PER_BUFFER);
        } else {
            std::array<f32, Streaming::SAMPLES_PER_BUFFER> temp_buffer{};
            stream.current_sample = audio::fillLoopingSampleWindow(wav.samples, {.first_sample = stream.first_sample, .last_sample = stream.last_sample},
                                                                   stream.current_sample, temp_buffer);
            if (!alCall(alBufferData, buf, format, temp_buffer.data(), Streaming::SAMPLES_PER_BUFFER * sizeof(f32), wav.sample_rate)) {
                return Result::OAL_ERROR;
            }
        }
    } else {
        usize to_copy = std::min(Streaming::SAMPLES_PER_BUFFER, remain);
        if (to_copy == 0) {
            return Result::OK;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) C buffer
        if (!alCall(alBufferData, buf, format, wav.samples.data() + stream.current_sample, to_copy * sizeof(f32), wav.sample_rate)) {
            return Result::OAL_ERROR;
        }
        stream.current_sample += to_copy;
    }

    if (!alCall(alSourceQueueBuffers, stream.source, 1, &buf)) {
        return Result::OAL_ERROR;
    }
    stream.current_buffer = (stream.current_buffer + 1) % Streaming::BUFFER_COUNT;
    stream.queued_buffer_count++;
    return Result::OK;
}

void AudioEngine::processCmdStop(const audio::cmd::StopStream& cmd) {
    if (cmd.id >= streaming_sources_.size()) {
        MLE_E("Invalid streaming source id: {}", cmd.id);
        return;
    }

    auto& stream = streaming_sources_.at(cmd.id);
    if (!stream.active) {
        MLE_T("Streaming source id {} is inactive", cmd.id);
        return;
    }
    if (cmd.fade_out_ms == 0) {
        std::ignore = stopStream(cmd.id);
        return;
    }
    if (stream.ramp.active && !stream.paused) {
        const auto now = std::chrono::steady_clock::now();
        stream.volume = audio::advanceRamp(stream.ramp, std::chrono::duration_cast<std::chrono::milliseconds>(now - stream.ramp_tick)).volume;
    }
    stream.ramp = audio::beginRamp(stream.volume, 0.0F, std::chrono::milliseconds{cmd.fade_out_ms}, audio::RampCompletion::STOP);
    stream.ramp_tick = std::chrono::steady_clock::now();
}

void AudioEngine::processCmdSetStreamParams(const audio::cmd::SetStreamParams& cmd) {
    if (cmd.id >= streaming_sources_.size()) {
        MLE_E("Invalid streaming source id: {}", cmd.id);
        return;
    }
    auto& stream = streaming_sources_.at(cmd.id);
    if (!stream.active) {
        MLE_T("Streaming source id {} is inactive", cmd.id);
        return;
    }
    const auto pitch = cmd.pitch ? audio::normalizeStreamPitch(*cmd.pitch) : std::optional<f32>{};
    if (cmd.pitch && !pitch) {
        MLE_W("Invalid stream pitch for source id: {}", cmd.id);
        return;
    }
    if (cmd.volume && !std::isfinite(*cmd.volume)) {
        MLE_W("Invalid stream volume for source id: {}", cmd.id);
        return;
    }
    if (pitch) {
        std::ignore = alCall(alSourcef, stream.source, AL_PITCH, *pitch);
    }
    if (!cmd.volume) {
        return;
    }
    const f32 target = std::clamp(*cmd.volume, 0.0F, 4.0F);
    if (cmd.fade_ms == 0) {
        stream.volume = target;
        stream.ramp = {};
        stream.ramp_tick = {};
        std::ignore = applyVolume(stream.source, stream.bus, stream.volume);
        return;
    }
    if (stream.ramp.active && !stream.paused) {
        const auto now = std::chrono::steady_clock::now();
        stream.volume = audio::advanceRamp(stream.ramp, std::chrono::duration_cast<std::chrono::milliseconds>(now - stream.ramp_tick)).volume;
    }
    stream.ramp = audio::beginRamp(stream.volume, target, std::chrono::milliseconds{cmd.fade_ms}, audio::RampCompletion::NONE);
    stream.ramp_tick = std::chrono::steady_clock::now();
}

void AudioEngine::processCmdPause(const audio::cmd::PauseStream& cmd) {
    if (cmd.id >= streaming_sources_.size()) {
        MLE_E("Invalid streaming source id: {}", cmd.id);
        return;
    }

    auto& stream = streaming_sources_.at(cmd.id);
    if (stream.active && stream.source != 0 && !stream.paused) {
        std::ignore = alCall(alSourcePause, stream.source);
        stream.paused = true;
    }
}

void AudioEngine::processCmdResume(const audio::cmd::ResumeStream& cmd) {
    if (cmd.id >= streaming_sources_.size()) {
        MLE_E("Invalid streaming source id: {}", cmd.id);
        return;
    }

    auto& stream = streaming_sources_.at(cmd.id);
    if (stream.active && stream.source != 0 && stream.paused) {
        std::ignore = alCall(alSourcePlay, stream.source);
        stream.paused = false;
        stream.ramp_tick = std::chrono::steady_clock::now();
    }
}

void AudioEngine::processCmdSetVolume(const audio::cmd::SetVolume& cmd) {
    if (cmd.bus >= BUS_COUNT) {
        MLE_W("Invalid bus index: {}", cmd.bus);
        return;
    }

    setBusVolumeLinear(cmd.bus, cmd.volume);

    for (usize i = 0; i < one_shot_sources_.size(); ++i) {
        const auto& source = one_shot_sources_[i];
        const auto& metadata = one_shot_voice_metadata_[i];
        if (cmd.bus == 0 || (metadata.bus == cmd.bus && metadata.priority > 0)) {
            std::ignore = applyVolume(source.source, metadata.bus, metadata.volume);
        }
    }

    for (auto& stream : streaming_sources_) {
        if (cmd.bus == 0 || (stream.bus == cmd.bus && stream.active)) {
            std::ignore = applyVolume(stream.source, stream.bus, stream.volume);
        }
    }
}

void AudioEngine::processCmdSetListener(const audio::cmd::SetListener& cmd) {
    std::ignore = cmd;
    std::ignore = device_;
    MLE_TODO;
}

void AudioEngine::processCmdSetDistanceParams(const audio::cmd::SetDistanceParams& cmd) {
    std::ignore = cmd;
    std::ignore = device_;
    MLE_TODO;
}

void AudioEngine::processCmdStopAll(const audio::cmd::StopAll& /*unused*/) {
    for (usize i = 0; i < streaming_sources_.size(); ++i) {
        std::ignore = stopStream(as<u8>(i));
    }

    for (usize i = 0; i < one_shot_sources_.size(); ++i) {
        auto& source = one_shot_sources_.at(i);
        auto& metadata = one_shot_voice_metadata_.at(i);
        if (metadata.priority > 0) {
            std::ignore = alCall(alSourceStop, source.source);
            audio::resetVoice(metadata);
        }
    }
}

void AudioEngine::processCmdSetBusVoicePolicy(const audio::cmd::SetBusVoicePolicy& cmd) {
    if (cmd.bus >= bus_voice_policies_.size()) {
        MLE_W("Invalid bus index for voice policy: {}", cmd.bus);
        return;
    }
    bus_voice_policies_.at(cmd.bus) = {.max_voices = cmd.max_voices, .protected_from_other_buses = cmd.protected_from_other_buses};
}

namespace {
void luaPlayOneShot(const sol::object& obj) {
    if (!obj.valid()) {
        MLE_W("Audio.playOneShot: invalid obj");
        return;
    }
    audio::cmd::PlayOneShot cmd;
    if (obj.is<std::string>()) {
        const std::string sound_name = obj.as<std::string>();
        cmd.sound_id = entt::hashed_string{sound_name.c_str()};
    } else if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();

        if (const auto name_r = table["name"]; lua::valid<std::string>(name_r)) {
            auto name = table["name"].get<std::string>();
            cmd.sound_id = entt::hashed_string{name.c_str()};
        } else {
            MLE_W("Audio.playOneShot: 'name' field is required in the table.");
            return;
        }

        if (const auto bus_r = table["bus"]; lua::valid<u8>(bus_r)) {
            cmd.params.bus = bus_r.get<u8>();
        }

        if (const auto volume_r = table["volume"]; lua::valid<f32>(volume_r)) {
            cmd.params.volume = volume_r.get<f32>();
        }

        if (const auto pitch_r = table["pitch"]; lua::valid<f32>(pitch_r)) {
            cmd.params.pitch = pitch_r.get<f32>();
        }

        if (const auto priority_r = table["priority"]; lua::valid<usize>(priority_r)) {
            cmd.priority = priority_r.get<usize>();
        }

    } else {
        MLE_E("Audio.playOneShot: invalid argument type, expected string or table.");
        return;
    }

    AudioEngine::i().enqueueCmd(cmd);
}

void luaSetVolume(u8 bus, f32 volume) {
    audio::cmd::SetVolume cmd;
    cmd.bus = bus;
    cmd.volume = volume;
    AudioEngine::i().enqueueCmd(cmd);
}

void luaSetBusVoicePolicy(const sol::object& obj) {
    if (!obj.valid() || !obj.is<sol::table>()) {
        MLE_W("Audio.setBusVoicePolicy: expected table.");
        return;
    }

    const auto table = obj.as<sol::table>();
    const auto bus_r = table["bus"];
    if (!lua::valid<u8>(bus_r)) {
        MLE_W("Audio.setBusVoicePolicy: valid 'bus' field is required.");
        return;
    }

    audio::cmd::SetBusVoicePolicy cmd;
    cmd.bus = bus_r.get<u8>();
    if (cmd.bus >= audio::BUS_COUNT) {
        MLE_W("Audio.setBusVoicePolicy: invalid bus index: {}", cmd.bus);
        return;
    }
    if (const auto max_voices_r = table["max_voices"]; lua::valid<u16>(max_voices_r)) {
        cmd.max_voices = max_voices_r.get<u16>();
    }
    if (const auto protected_r = table["protected_from_other_buses"]; lua::valid<bool>(protected_r)) {
        cmd.protected_from_other_buses = protected_r.get<bool>();
    }
    AudioEngine::i().enqueueCmd(cmd);
}

void luaStopAll() {
    audio::cmd::StopAll cmd;
    AudioEngine::i().enqueueCmd(cmd);
}

void luaStartStream(const sol::object& obj) {
    if (!obj.valid()) {
        MLE_E("Audio.startStream: invalid obj");
        return;
    }
    audio::cmd::StartStream cmd;
    if (obj.is<std::string>()) {
        const std::string sound_name = obj.as<std::string>();
        cmd.sound_id = entt::hashed_string{sound_name.c_str()};
        cmd.params.bus = 0;
        cmd.id = 0;
    } else if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();

        if (const auto name_r = table["name"]; lua::valid<std::string>(name_r)) {
            auto name = table["name"].get<std::string>();
            cmd.sound_id = entt::hashed_string{name.c_str()};
        } else {
            MLE_W("Audio.startStream: 'name' field is required in the table.");
            return;
        }

        if (const auto id_r = table["id"]; lua::valid<u8>(id_r)) {
            cmd.id = id_r.get<u8>();
        } else {
            MLE_W("Audio.startStream: 'id' field is required in the table.");
            return;
        }

        if (const auto bus_r = table["bus"]; lua::valid<u8>(bus_r)) {
            cmd.params.bus = bus_r.get<u8>();
        }

        if (const auto volume_r = table["volume"]; lua::valid<f32>(volume_r)) {
            cmd.params.volume = volume_r.get<f32>();
        }

        if (const auto pitch_r = table["pitch"]; lua::valid<f32>(pitch_r)) {
            cmd.params.pitch = pitch_r.get<f32>();
        }

        if (const auto looping_r = table["looping"]; lua::valid<bool>(looping_r)) {
            cmd.loop = looping_r.get<bool>();
        }

        if (const auto fade_r = table["fade_in_ms"]; lua::valid<u32>(fade_r)) {
            cmd.params.fade_in_ms = fade_r.get<u32>();
        }

    } else {
        MLE_E("Audio.startStream: invalid argument type, expected table.");
        return;
    }

    AudioEngine::i().enqueueCmd(cmd);
}

void luaSetStreamParams(const sol::object& obj) {
    if (!obj.valid() || !obj.is<sol::table>()) {
        MLE_W("Audio.setStreamParams: expected table");
        return;
    }
    const auto table = obj.as<sol::table>();
    const auto id_r = table["id"];
    if (!lua::valid<u8>(id_r)) {
        MLE_W("Audio.setStreamParams: 'id' field is required in the table.");
        return;
    }
    audio::cmd::SetStreamParams cmd;
    cmd.id = id_r.get<u8>();
    if (const auto volume_r = table["volume"]; lua::valid<f32>(volume_r)) {
        cmd.volume = volume_r.get<f32>();
    }
    if (const auto pitch_r = table["pitch"]; lua::valid<f32>(pitch_r)) {
        cmd.pitch = pitch_r.get<f32>();
    }
    if (const auto fade_r = table["fade_ms"]; lua::valid<u32>(fade_r)) {
        cmd.fade_ms = fade_r.get<u32>();
    }
    AudioEngine::i().enqueueCmd(cmd);
}

void luaStopStream(u8 id, sol::optional<u32> fade_out_ms) {
    audio::cmd::StopStream cmd;
    cmd.id = id;
    cmd.fade_out_ms = fade_out_ms.value_or(0);
    AudioEngine::i().enqueueCmd(cmd);
}

void luaPauseStream(u8 id) {
    audio::cmd::PauseStream cmd;
    cmd.id = id;
    AudioEngine::i().enqueueCmd(cmd);
}

void luaResumeStream(u8 id) {
    audio::cmd::ResumeStream cmd;
    cmd.id = id;
    AudioEngine::i().enqueueCmd(cmd);
}

void luaLoadSound(const sol::object& obj) {
    if (!obj.valid()) {
        return;
    }
    audio::cmd::Load cmd;
    if (obj.is<std::string>()) {
        const std::string sound_name = obj.as<std::string>();
        cmd.name = sound_name;
        cmd.stream = false;
    } else if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();

        if (const auto name_r = table["name"]; lua::valid<std::string>(name_r)) {
            auto name = table["name"].get<std::string>();
            cmd.name = name;
        } else {
            MLE_W("Audio.loadSound: 'name' field is required in the table.");
            return;
        }

        if (const auto stream_r = table["stream"]; lua::valid<bool>(stream_r)) {
            cmd.stream = stream_r.get<bool>();
        }
    } else {
        MLE_E("Audio.loadSound: invalid argument type, expected string or table.");
        return;
    }

    AudioEngine::i().enqueueCmd(cmd);
}

f32 luaGetVolume(u8 bus) {
    return AudioEngine::i().getVolume(bus);
}
}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity) Not complex tho
void AudioEngine::addLuaBinding() {
    auto& c_table = Client::i().getCTable();

    c_table["Audio"] = Client::i().lua().createTable();

    c_table["Audio"]["playOneShot"] = luaPlayOneShot;
    c_table["Audio"]["setVolume"] = luaSetVolume;
    c_table["Audio"]["setBusVoicePolicy"] = luaSetBusVoicePolicy;
    c_table["Audio"]["getVolume"] = luaGetVolume;
    c_table["Audio"]["stopAll"] = luaStopAll;
    c_table["Audio"]["startStream"] = luaStartStream;
    c_table["Audio"]["setStreamParams"] = luaSetStreamParams;
    c_table["Audio"]["stopStream"] = luaStopStream;
    c_table["Audio"]["pauseStream"] = luaPauseStream;
    c_table["Audio"]["resumeStream"] = luaResumeStream;
    c_table["Audio"]["loadSound"] = luaLoadSound;
};
}  // namespace mle
