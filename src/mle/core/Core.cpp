#include "Core.h"

#include "Logger.h"
#include "RuntimeConfig.h"

namespace mle {
void init(const CoreInitInfo& ii) {
    RuntimeConfig::i().parseArgs(ii.argc, ii.argv);
    Logger::i().init();
    RuntimeConfig::i().logAll();
}
}  // namespace mle
