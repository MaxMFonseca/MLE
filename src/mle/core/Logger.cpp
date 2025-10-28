#include "Logger.h"

#include "mle/utils/String.h"

namespace mle {
void Logger::init() {
    static bool initialized = false;
    assert(!initialized && "Logger already initialized");
    if (initialized) {
        return;
    }

    // File sink
    file_sink_ = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/latest.log", true);
    file_sink_->set_pattern("[%L][%T.%e][%@][%!] %v");
    auto file_level = as<LogLevel>(mle::RuntimeConfig::i().getInt("log.level.file").value_or(as<int>(MAX_LOG_LEVEL)));
    file_sink_->set_level(static_cast<spdlog::level::level_enum>(file_level));

    // Stdout sink
    stdout_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    stdout_sink_->set_pattern("[%L][%T.%e][%s.%#][%!] %v%$");
    auto stdout_level = as<LogLevel>(mle::RuntimeConfig::i().getInt("log.level.stdout").value_or(as<int>(DEFAULT_LOG_LEVEL_STDOUT)));
    stdout_sink_->set_level(static_cast<spdlog::level::level_enum>(stdout_level));

    std::vector<spdlog::sink_ptr> sinks{file_sink_, stdout_sink_};

    file_level_listener_.setKey("log.level.file")
        .setCallback([this](const std::string& value) {
            int leveli = strTo<int>(value).value_or(as<int>(MAX_LOG_LEVEL));
            file_sink_->set_level(as<spdlog::level::level_enum>(leveli));
            MLE_I("file LogLevel changed {}", leveli);
            return false;
        })
        .listen();

    stdout_level_listener_.setKey("log.level.stdout")
        .setCallback([this](const std::string& value) {
            int leveli = strTo<int>(value).value_or(as<int>(DEFAULT_LOG_LEVEL_STDOUT));
            stdout_sink_->set_level(as<spdlog::level::level_enum>(leveli));
            MLE_I("stdout LogLevel changed {}", leveli);
            return false;
        })
        .listen();

    auto default_logger = std::make_shared<spdlog::logger>("MLELogger", sinks.begin(), sinks.end());
    default_logger->set_level(static_cast<spdlog::level::level_enum>(MAX_LOG_LEVEL));

    spdlog::set_default_logger(default_logger);
    spdlog::set_error_handler([](std::string const& msg) { spdlog::error(msg); });

    MLE_I("Logger initialized. MAX_LOG_LEVEL:{} FILE_LOG_LEVEL:{} STDOUT_LOG_LEVEL: {}", MAX_LOG_LEVEL, file_level, stdout_level);

    initialized = true;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static) not now
void Logger::shutdown() {
    MLE_I("Shutting down logger.");
    spdlog::shutdown();
}
}  // namespace mle
