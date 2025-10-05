#include "Core.h"

#include "Logger.h"
#include "RuntimeConfig.h"
#include "ThreadPool.h"

namespace mle::core {
void init(const InitInfo& ii) {
    RuntimeConfig::i().parseArgs(ii.argc, ii.argv);
    Logger::i().init();
    RuntimeConfig::i().logAll();
    ThreadPool::i().init();
}

void unrecoverable(const std::string& msg) {
    MLE_C("Unrecoverable error: {}", msg);
    std::abort();
}
}  // namespace mle::core
