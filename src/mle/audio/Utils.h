#pragma once

#include "mle/utils/File.h"

namespace mle {
struct WavData {
    std::vector<f32> samples;
    u32 channels = 0;
    u32 sample_rate = 0;
};

Expected<WavData> loadWavFile(const Path& path);
}  // namespace mle
