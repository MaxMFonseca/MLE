#include "Utils.h"

#include <dr_wav.h>

#include "mle/core/Logger.h"

namespace mle {
mle::Expected<WavData> loadWavFile(const Path& path) {
    drwav wav{};
    if (!drwav_init_file(&wav, path.c_str(), nullptr)) {
        MLE_E("Failed to open WAV file: {}", path);
        return std::unexpected(Result::FAILED_TO_OPEN);
    }

    WavData ret;

    ret.channels = wav.channels;
    ret.sample_rate = wav.sampleRate;

    usize total_samples = as<usize>(wav.totalPCMFrameCount) * wav.channels;
    ret.samples.resize(total_samples);

    usize samples_read = drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, ret.samples.data());
    if (samples_read == 0) {
        MLE_E("Failed to read samples from WAV file: {}", path);
        drwav_uninit(&wav);
        return std::unexpected(Result::FAILED_TO_OPEN);
    }

    drwav_uninit(&wav);
    return ret;
}

}  // namespace mle
