#pragma once

#include <thread>

#include "Types.h"
#include "mle/utils/Utils.h"

namespace mle {
class AudioEngine {
    MLE_SINGLETON(AudioEngine)
  public:
    Result init();
    void shutdown();

    Expected<std::vector<std::string>> getAvailableDevices();

  private:
    void runLoop(std::stop_token st);

    void logAvailableDevices();

  private:
    ALCdevice* device_{};
    ALCcontext* context_{};

    std::jthread run_thread_{};
};
}  // namespace mle
