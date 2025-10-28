#include "Core.h"

#include "Logger.h"
#include "RuntimeConfig.h"
#include "ThreadPool.h"
#include "mle/client/Client.h"
#include "mle/core/PerfTracker.h"
#include "mle/renderer/Renderer.h"

namespace mle {
namespace {
void addEngineRTCKeys() {
    auto& rtc = RuntimeConfig::i();
    rtc.addKey("log.level.file", "Log level for file logging (0=trace, 1=debug, 2=info, 3=warn, 4=err, 5=critical, 6=off)");
    rtc.addKey("log.level.stdout", "Log level for stdout logging (0=trace, 1=debug, 2=info, 3=warn, 4=err, 5=critical, 6=off)");

    // not permanent stuff here
    rtc.addKey("renderer.swapchain.present_mode", "Swapchain present mode (e.g. 'FIFO', 'MAILBOX', 'IMMEDIATE')");
    rtc.addKey("renderer.swapchain.triple_buffer", "Enable triple buffering for swapchain (true/false)");
    rtc.addKey("renderer.swapchain.default_clear_color", "Default clear color for swapchain (e.g. 'ZERO', 'WHITE', 'RED', etc.)");
    rtc.addKey("renderer.target_fps", "Target FPS for the renderer (0 for unlimited)");
}
}  // namespace

void Core::init(const InitInfo& ii) {
    running_time_.reset();

    addEngineRTCKeys();
    RuntimeConfig::i().parseArgs(ii.argc, ii.argv);
    Logger::i().init();
    RuntimeConfig::i().logAllValues();
    RuntimeConfig::i().logAllDescriptions();
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
