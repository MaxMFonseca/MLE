#include "Window.h"

#include "GLFW/glfw3.h"
#include "mle/common/Logger.h"
#include "mle/lua/Utils.h"
#include "mle/window/Types.h"
#include "mle/window/UserInputManager.h"

namespace mle::window {
namespace {
class Impl {
  public:
    inline Result init(const CI& ci);
    inline void update();
    inline void shutdown();

    auto& getCurrentConfig() { return current_config_; }
    ED& getED() { return ed_; }
    UserInputManager& getUIM() { return uim_; }
    GLFWWindowRef getGlfwWindowRef() { return window_; }
    vec2i getSize() const { return current_config_.size; }
    f32 getAspectRatio() const { return static_cast<f32>(current_config_.size.x) / static_cast<f32>(current_config_.size.y); }

  private:
    inline void handleGLFWErrors();
    inline void applyLuaOverrides(const sol::table& table);
    inline void registerCallbacks();

    static void onWindowClose(GLFWWindowRef glfw_window);
    static void onWindowResize(GLFWWindowRef glfw_window, int width, int height);
    static void onWindowIconify(GLFWWindowRef glfw_window, int iconified);
    static void onCursorMove(GLFWWindowRef glfw_window, double x, double y);
    static void onMouseScroll(GLFWWindowRef glfw_window, double x, double y);
    static void onMouseButton(GLFWWindowRef glfw_window, int button, int action, int mods);
    static void onKey(GLFWWindowRef glfw_window, int key, int scancode, int action, int mods);
    static void onChar(GLFWWindowRef glfw_window, unsigned codepoint);

  private:
    GLFWWindowRef window_ = nullptr;
    Config current_config_;
    Config target_config_;
    bool target_config_changed_ = false;

    ED ed_;
    UserInputManager uim_;
};

Result Impl::init(const CI& ci) {
    MLE_I("Initializing Window Module");

    MLE_T("GLFW");
    if (!glfwInit()) {
        MLE_E("glfwInit() failed!");
        return Result::INIT_FAILED;
    }
    MLE_T("GLFW initialized successfully");

    glfwSetErrorCallback([]([[maybe_unused]] int code, [[maybe_unused]] const char* desc) {
        MLE_UNREACHABLE_LOG("GLFW error : {}: {}", code, desc);  // NOLINT(bugprone-lambda-function-name)
    });

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    target_config_ = ci.target_config;
    if (lua::valid<sol::table>(ci.table)) {
        applyLuaOverrides(ci.table);
    }

    window_ = glfwCreateWindow(target_config_.size.x, target_config_.size.y, target_config_.title.c_str(), nullptr, nullptr);
    if (!window_) {
        MLE_E("glfwCreateWindow() failed!");
        return Result::INIT_FAILED;
    }

    current_config_.title = target_config_.title;
    glfwGetWindowSize(window_, &current_config_.size.x, &current_config_.size.y);

    registerCallbacks();

    MLE_I("Window created successfully! name: {}, size: {}x{}", current_config_.title, current_config_.size.x, current_config_.size.y);
    MLE_D("Module initialized successfully!");
    return Result::OK;
}

void Impl::applyLuaOverrides(const sol::table& table) {
    lua::tryGetKey(table, "title", target_config_.title);
    lua::tryGetKey(table, "width", target_config_.size.x);
    lua::tryGetKey(table, "height", target_config_.size.y);
    lua::tryAs(table["size"], target_config_.size);
}

void Impl::registerCallbacks() {
    glfwSetWindowUserPointer(window_, this);

    glfwSetWindowCloseCallback(window_, onWindowClose);
    glfwSetWindowSizeCallback(window_, onWindowResize);
    glfwSetWindowIconifyCallback(window_, onWindowIconify);

    glfwSetCursorPosCallback(window_, onCursorMove);
    glfwSetMouseButtonCallback(window_, onMouseButton);
    glfwSetScrollCallback(window_, onMouseScroll);
    glfwSetKeyCallback(window_, onKey);
    glfwSetCharCallback(window_, onChar);
}

void Impl::shutdown() {
    MLE_I("Shutting down Window Module");
    glfwDestroyWindow(window_);
    glfwTerminate();
    MLE_D("Module shut down successfully!");
}

void Impl::update() {
    glfwPollEvents();
    uim_.update();
}

auto& windowToSelf(GLFWWindowRef glfw_window) {
    return asRef<Impl>(glfwGetWindowUserPointer(glfw_window));
}

void Impl::onWindowClose(GLFWWindowRef glfw_window) {
    auto& self = windowToSelf(glfw_window);
    self.ed_.dispatch(events::WindowClose{});
}

void Impl::onWindowResize(GLFWWindowRef glfw_window, int width, int height) {
    auto& self = windowToSelf(glfw_window);
    MLE_I("Window resized to {}x{}", width, height);
    self.current_config_.size = {width, height};
    self.ed_.dispatch(events::WindowResize{{width, height}});
}

void Impl::onWindowIconify(GLFWWindowRef glfw_window, int iconified) {
    auto& self = windowToSelf(glfw_window);
    MLE_I("Window iconify: {}", iconified);
    self.ed_.dispatch(events::WindowIconify{as<bool>(iconified)});
}

void Impl::onCursorMove(GLFWWindowRef glfw_window, double x, double y) {
    auto& self = windowToSelf(glfw_window);
    self.uim_.setCursorPos({x, y});
}

void Impl::onMouseScroll(GLFWWindowRef glfw_window, [[maybe_unused]] double x, double y) {
    auto& self = windowToSelf(glfw_window);
    self.uim_.setScrollOffset(y);
}

void Impl::onMouseButton(GLFWWindowRef glfw_window, int button, int action, int /*mods*/) {
    auto& self = windowToSelf(glfw_window);
    if (action == GLFW_PRESS) {
        self.uim_.setPressed(castGlfwMouseButtonToKey(button));
    } else if (action == GLFW_RELEASE) {
        self.uim_.setReleased(castGlfwMouseButtonToKey(button));
    }
}

void Impl::onKey(GLFWWindowRef glfw_window, int key, int /*scancode*/, int action, int /*mods*/) {
    auto& self = windowToSelf(glfw_window);
    if (action == GLFW_PRESS) {
        self.uim_.setPressed(castGlfwKeyToKey(key));
    } else if (action == GLFW_RELEASE) {
        self.uim_.setReleased(castGlfwKeyToKey(key));
    }
}

void Impl::onChar(GLFWWindowRef glfw_window, unsigned codepoint) {
    auto& self = windowToSelf(glfw_window);
    self.uim_.pushChar(codepoint);
}

// TODO: I will probably allocate this at a linear allocator along the other core singletons in the future
std::unique_ptr<Impl> i_;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
}  // namespace

Result init(const CI& ci) {
    MLE_ASSERT(!i_);
    i_ = std::make_unique<Impl>();
    auto result = i_->init(ci);
    if (result != Result::OK) {
        MLE_C("Window initialization failed with error: {}", result);
        i_.reset();
    }
    return result;
}

void update() {
    MLE_ASSERT(i_);
    i_->update();
}

void shutdown() {
    if (i_) {
        MLE_I("Shutting down Window Module");
        i_->shutdown();
        i_.reset();
    }
}

ED& getED() {
    MLE_ASSERT(i_);
    return i_->getED();
}

UserInputManager& getUIM() {
    MLE_ASSERT(i_);
    return i_->getUIM();
}

GLFWWindowRef getGlfwWindowRef() {
    MLE_ASSERT(i_);
    return i_->getGlfwWindowRef();
}

vec2i getSize() {
    MLE_ASSERT(i_);
    return i_->getSize();
}

const Config& getCurrentConfig() {
    MLE_ASSERT(i_);
    return i_->getCurrentConfig();
}
}  // namespace mle::window
