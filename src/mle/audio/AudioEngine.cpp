#include "AudioEngine.h"

#include <AL/al.h>

#include <expected>
#include <source_location>

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
bool alCallImpl(std::source_location loc, F&& f, Args&&... args) {
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
bool alcCallImpl(std::source_location loc, ALCdevice* device, F&& f, Args&&... args) {
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

// NOLINTNEXTLINE(performance-unnecessary-value-param) stop_token is small and cheap to copy
void AudioEngine::runLoop(std::stop_token st) {
    if (const auto r = alcCall(alcMakeContextCurrent, context_); !r.has_value()) {
        MLE_C("Failed to make OpenAL context current: {}", r.error());
        return;
    }

    while (!st.stop_requested()) {
        {
            MLE_PERF_SCOPE("AudioEngine");
        }
        std::this_thread::sleep_for(100ms);
    }
}
}  // namespace mle
