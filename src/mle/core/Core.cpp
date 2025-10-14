#include "Core.h"

#include "Logger.h"
#include "RuntimeConfig.h"
#include "ThreadPool.h"
#include "mle/client/Client.h"
#include "mle/core/PerfTracker.h"
#include "mle/renderer/Renderer.h"

namespace mle {
void Core::init(const InitInfo& ii) {
    running_time_.reset();

    RuntimeConfig::i().parseArgs(ii.argc, ii.argv);
    Logger::i().init();
    RuntimeConfig::i().logAll();
    ThreadPool::i().init();
    PerfTracker::i().init();

    if constexpr (IS_CLIENT) {
        Client::i().init();
    }

    Renderer::i().init();

    MLE_I("MLE Core initialized successfully after {}s", running_time_.elapsedSecFloat());
}

void Core::shutdown() {
    Renderer::i().shutdown();

    PerfTracker::i().shutdown();
    ThreadPool::i().shutdown();

    MLE_I("MLE Core shutdown completed after {}s", running_time_.elapsedSecFloat());
}

void Core::unrecoverable(const std::string& msg) {
    MLE_C("Unrecoverable error: {}, time: {}s", msg, running_time_.elapsedSecFloat());
    std::abort();
}
}  // namespace mle
