#include "mle/audio/StreamState.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "mle/core/Assert.h"

namespace mle::audio {
namespace {
usize samplesAtMs(u64 samples_per_second, u32 milliseconds, usize limit) {
    const u64 whole_seconds = milliseconds / 1000;
    if (samples_per_second != 0 && whole_seconds > limit / samples_per_second) {
        return limit;
    }

    const u64 whole_samples = whole_seconds * samples_per_second;
    const u64 partial_samples = samples_per_second * (milliseconds % 1000) / 1000;
    if (partial_samples >= limit - whole_samples) {
        return limit;
    }
    return whole_samples + partial_samples;
}
}  // namespace

Expected<SampleWindow> computeSampleWindow(u32 sample_rate, u16 channels, usize total_samples, u32 offset_ms, u32 duration_ms) {
    if (sample_rate == 0 || channels == 0) {
        return std::unexpected(Result::INVALID_ARGUMENT);
    }

    const u64 samples_per_second = static_cast<u64>(sample_rate) * channels;
    const usize first_sample = samplesAtMs(samples_per_second, offset_ms, total_samples);
    if (first_sample >= total_samples) {
        return std::unexpected(Result::OUT_OR_RANGE);
    }

    if (duration_ms == 0) {
        return SampleWindow{.first_sample = first_sample, .last_sample = total_samples};
    }

    const usize remaining = total_samples - first_sample;
    const usize duration_samples = samplesAtMs(samples_per_second, duration_ms, remaining);
    if (duration_samples == 0) {
        return std::unexpected(Result::OUT_OR_RANGE);
    }
    return SampleWindow{.first_sample = first_sample, .last_sample = first_sample + duration_samples};
}

std::optional<f32> normalizeStreamPitch(f32 pitch) {
    if (!std::isfinite(pitch) || pitch <= 0.0F) {
        return std::nullopt;
    }
    return std::clamp(pitch, 0.01F, 4.0F);
}

Expected<ValidatedStreamStart> validateStreamStart(u32 sample_rate, u16 channels, usize total_samples, u8 bus, f32 volume, f32 pitch, u32 offset_ms,
                                                   u32 duration_ms, usize samples_per_buffer) {
    if (bus >= BUS_COUNT || !std::isfinite(volume)) {
        return std::unexpected(Result::OUT_OR_RANGE);
    }
    const auto normalized_pitch = normalizeStreamPitch(pitch);
    if (!normalized_pitch) {
        return std::unexpected(Result::OUT_OR_RANGE);
    }
    const auto window = computeSampleWindow(sample_rate, channels, total_samples, offset_ms, duration_ms);
    if (!window) {
        return std::unexpected(window.error());
    }
    if (total_samples <= samples_per_buffer) {
        return std::unexpected(Result::OUT_OR_RANGE);
    }
    return ValidatedStreamStart{.window = *window, .pitch = *normalized_pitch, .volume = std::clamp(volume, 0.0F, 4.0F)};
}

usize fillLoopingSampleWindow(std::span<const f32> samples, SampleWindow window, usize current_sample, std::span<f32> destination) {
    MLE_ASSERT(window.first_sample < window.last_sample);
    MLE_ASSERT(window.last_sample <= samples.size());
    MLE_ASSERT(current_sample >= window.first_sample && current_sample < window.last_sample);

    usize written = 0;
    while (written < destination.size()) {
        const usize chunk_size = std::min(window.last_sample - current_sample, destination.size() - written);
        std::ranges::copy(samples.subspan(current_sample, chunk_size), destination.subspan(written).begin());
        written += chunk_size;
        current_sample += chunk_size;
        if (current_sample == window.last_sample) {
            current_sample = window.first_sample;
        }
    }
    return current_sample;
}

usize advanceLoopingSampleCursor(SampleWindow window, usize current_sample, usize sample_count) {
    MLE_ASSERT(window.first_sample < window.last_sample);
    MLE_ASSERT(current_sample >= window.first_sample && current_sample < window.last_sample);

    const usize window_size = window.last_sample - window.first_sample;
    const usize current_offset = current_sample - window.first_sample;
    const usize advance = sample_count % window_size;
    const usize distance_to_end = window_size - current_offset;
    const usize next_offset = advance >= distance_to_end ? advance - distance_to_end : current_offset + advance;
    return window.first_sample + next_offset;
}

VolumeRamp beginRamp(f32 current, f32 target, std::chrono::milliseconds duration, RampCompletion completion) {
    if (duration <= std::chrono::milliseconds::zero()) {
        return VolumeRamp{.start = current,
                          .current = target,
                          .target = target,
                          .duration = duration,
                          .elapsed = std::chrono::milliseconds::zero(),
                          .completion = completion,
                          .active = false};
    }
    return VolumeRamp{.start = current,
                      .current = current,
                      .target = target,
                      .duration = duration,
                      .elapsed = std::chrono::milliseconds::zero(),
                      .completion = completion,
                      .active = true};
}

RampAdvance advanceRamp(VolumeRamp& ramp, std::chrono::milliseconds delta) {
    if (!ramp.active) {
        return {.volume = ramp.current, .completion = std::exchange(ramp.completion, RampCompletion::NONE)};
    }

    delta = std::max(delta, std::chrono::milliseconds::zero());
    const auto remaining = ramp.duration - ramp.elapsed;
    ramp.elapsed = delta >= remaining ? ramp.duration : ramp.elapsed + delta;
    if (ramp.elapsed == ramp.duration) {
        ramp.current = ramp.target;
        ramp.active = false;
        return {.volume = ramp.current, .completion = std::exchange(ramp.completion, RampCompletion::NONE)};
    }

    const f32 progress = static_cast<f32>(ramp.elapsed.count()) / static_cast<f32>(ramp.duration.count());
    ramp.current = ramp.start + ((ramp.target - ramp.start) * progress);
    return {.volume = ramp.current, .completion = RampCompletion::NONE};
}
}  // namespace mle::audio
