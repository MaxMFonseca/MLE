#pragma once

#include <chrono>
#include <span>

#include "mle/audio/VoiceAllocator.h"
#include "mle/core/Result.h"
#include "mle/utils/Types.h"

namespace mle::audio {
struct SampleWindow {
    usize first_sample;
    usize last_sample;

    bool operator==(const SampleWindow&) const = default;
};

Expected<SampleWindow> computeSampleWindow(u32 sample_rate, u16 channels, usize total_samples, u32 offset_ms, u32 duration_ms);
std::optional<f32> normalizeStreamPitch(f32 pitch);

struct ValidatedStreamStart {
    SampleWindow window;
    f32 pitch;
    f32 volume;
};

Expected<ValidatedStreamStart> validateStreamStart(u32 sample_rate, u16 channels, usize total_samples, u8 bus, f32 volume, f32 pitch, u32 offset_ms,
                                                   u32 duration_ms, usize samples_per_buffer);
usize advanceLoopingSampleCursor(SampleWindow window, usize current_sample, usize sample_count);
usize fillLoopingSampleWindow(std::span<const f32> samples, SampleWindow window, usize current_sample, std::span<f32> destination);

enum class RampCompletion : u8 {
    NONE,
    STOP,
};

struct VolumeRamp {
    f32 start;
    f32 current;
    f32 target;
    std::chrono::milliseconds duration;
    std::chrono::milliseconds elapsed;
    RampCompletion completion;
    bool active;
};

struct RampAdvance {
    f32 volume;
    RampCompletion completion;
};

VolumeRamp beginRamp(f32 current, f32 target, std::chrono::milliseconds duration, RampCompletion completion);
RampAdvance advanceRamp(VolumeRamp& ramp, std::chrono::milliseconds delta);
}  // namespace mle::audio
