#pragma once

#include <AL/al.h>

#include "mle/utils/File.h"

namespace mle {
struct WavData {
    std::vector<f32> samples;
    u32 channels = 0;
    u32 sample_rate = 0;
};

Expected<WavData> loadWavFile(const Path& path);

constexpr float dbToLinear(float db) {
    return std::pow(10.0F, db / 20.0F);
}

inline float linearToDb(float g) {
    if (g <= 0.000001F) {
        return -120.0F;
    }
    return 20.0F * std::log10(g);
}

}  // namespace mle
