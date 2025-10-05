#include "Core.h"

#include "Logger.h"
#include "RuntimeConfig.h"
#include "ThreadPool.h"

namespace mle {
void Core::init(const InitInfo& ii) {
    running_time_.reset();

    RuntimeConfig::i().parseArgs(ii.argc, ii.argv);
    Logger::i().init();
    RuntimeConfig::i().logAll();
    ThreadPool::i().init();

    MLE_I("MLE Core initialized successfully after {}s", running_time_.elapsedSecFloat());
}

void Core::unrecoverable(const std::string& msg) {
    MLE_C("Unrecoverable error: {}, time: {}s", msg, running_time_.elapsedSecFloat());
    std::abort();
}
}  // namespace mle
