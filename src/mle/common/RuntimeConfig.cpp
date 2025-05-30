#include "RuntimeConfig.h"

#include <iostream>
#include <map>
#include <vulkan/vulkan.hpp>

#include "mle/common/Types.h"

namespace mle::runtime_config {
using VecIt = std::vector<std::string>::iterator;

#ifdef MLE_RUNTIME_CONFIG_LOG_LEVEL_STDOUT
namespace {
std::vector<std::function<void(LogLevel)>> listeners_log_level_stdout_;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
LogLevel log_level_stdout_ = DEFAULT_LOG_LEVEL_STDOUT;                   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void configOptionLogLevelStdout(VecIt it, usize argc) {
    assert(argc == 1 && "Expected 1 argument for log level");
    std::string s = *(it + 1);
    if (s == "OFF") {
        setLogLevelStdout(LogLevel::OFF);
    } else if (s == "C" || s == "CRITICAL" || s == "c" || s == "critical") {
        setLogLevelStdout(LogLevel::CRITICAL);
    } else if (s == "E" || s == "ERROR" || s == "e" || s == "error") {
        setLogLevelStdout(LogLevel::ERROR);
    } else if (s == "W" || s == "WARN" || s == "w" || s == "warn") {
        setLogLevelStdout(LogLevel::WARN);
    } else if (s == "I" || s == "INFO" || s == "i" || s == "info") {
        setLogLevelStdout(LogLevel::INFO);
    } else if (s == "D" || s == "DEBUG" || s == "d" || s == "debug") {
        setLogLevelStdout(LogLevel::DEBUG);
    } else if (s == "T" || s == "TRACE" || s == "ALL" || s == "t" || s == "trace" || s == "all") {
        setLogLevelStdout(LogLevel::TRACE);
    } else {
        std::cout << "Unknown log level: " << s << "\n";
        setLogLevelStdout(LogLevel::TRACE);
    }
}
}  // namespace

LogLevel getLogLevelStdout() {
    return log_level_stdout_;
}

void setLogLevelStdout(LogLevel level) {
    log_level_stdout_ = level;
    for (auto& fn : listeners_log_level_stdout_) {
        fn(level);
    }
}

void signListenerLogLevelStdout(std::function<void(LogLevel)>&& fn) {
    listeners_log_level_stdout_.emplace_back(std::move(fn));
}

#endif

#ifdef MLE_RUNTIME_CONFIG_RENDER_ELEMENT_PADDING
namespace {
bool render_element_padding_ = false;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void configOptionRenderElementPadding(VecIt it, usize argc) {
    if (argc) {
        assert(argc == 1);
        std::string s = *(it + 1);
        if (s == "true" || s == "1") {
            setRenderElementPadding(true);
        } else if (s == "false" || s == "0") {
            setRenderElementPadding(false);
        } else {
            assert(false);
        }
    } else {
        setRenderElementPadding(true);
    }
}
}  // namespace

bool getRenderElementPadding() {
    return render_element_padding_;
}

void setRenderElementPadding(bool render_element_padding) {
    render_element_padding_ = render_element_padding;
}

#endif

#ifdef MLE_RUNTIME_CONFIG_LOG_LEVEL_FILE
namespace {
LogLevel log_level_file_ = MAX_LOG_LEVEL;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
}  // namespace

LogLevel getLogLevelFile() {
    return log_level_file_;
}

void setLogLevelFile(LogLevel level) {
    log_level_file_ = level;
}
#endif

#ifdef MLE_RUNTIME_CONFIG_PRESENT_MODE
namespace {
vk::PresentModeKHR present_mode_ = vk::PresentModeKHR::eFifo;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void configOptionPresentMode(VecIt it, [[maybe_unused]] usize argc) {
    assert(argc == 1);
    std::string s = *(it + 1);
    if (s == "fifo") {
        setPresentMode(vk::PresentModeKHR::eFifo);
        return;
    }
    if (s == "fifo_relaxed") {
        setPresentMode(vk::PresentModeKHR::eFifoRelaxed);
        return;
    }
    if (s == "mailbox") {
        setPresentMode(vk::PresentModeKHR::eMailbox);
        return;
    }
    if (s == "immediate") {
        setPresentMode(vk::PresentModeKHR::eImmediate);
        return;
    }
}
};  // namespace

vk::PresentModeKHR getPresentMode() {
    return present_mode_;
}

void setPresentMode(vk::PresentModeKHR present_mode) {
    present_mode_ = present_mode;
}

#endif

void init(int argc, char** argv, const std::vector<ConfigOptionEntry>& user_options) {
    if (argc == 1) {
        return;
    }

    std::map<std::string, ConfigOptionFunction> config_options{};
    for (const auto& [opt, fn] : user_options) {
        config_options[opt] = fn;
    }
#ifdef MLE_RUNTIME_CONFIG_LOG_LEVEL_STDOUT
    config_options["log_level_stdout"] = configOptionLogLevelStdout;
#endif
#ifdef MLE_RUNTIME_CONFIG_RENDER_ELEMENT_PADDING
    config_options["render_element_padding"] = configOptionRenderElementPadding;
#endif
#ifdef MLE_RUNTIME_CONFIG_PRESENT_MODE
    config_options["present_mode"] = configOptionPresentMode;
#endif

    std::cout << "Parsing " << argc - 1 << " args: ";
    std::vector<std::string> args{};
    args.reserve(static_cast<usize>(argc - 1));
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::cout << args.back() << " ";
    }
    std::cout << "\n";

    for (auto it = args.begin(); it != args.end();) {
        assert(it->starts_with("--"));
        auto it0 = it;
        std::string opt = it->substr(2);
        ++it;
        usize arg_count = 0;
        while (it != args.end() && !it->starts_with("--")) {
            ++it;
            arg_count += 1;
        }
        auto fn_it = config_options.find(opt);
        if (fn_it != config_options.end()) {
            fn_it->second(it0, arg_count);
        } else {
            std::cout << "Unknown option: " << opt << "\n";
            assert(false && "Unknown option");
        }
    }
}
}  // namespace mle::runtime_config
