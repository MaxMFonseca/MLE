#include "Window.h"

#include <SDL3/SDL.h>

#include <iostream>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_vulkan.h"
#include "UserInputManager.h"
#include "mle/core/Assert.h"
#include "mle/core/Core.h"
#include "mle/utils/String.h"
#include "mle/window/KeyUtils.h"
#include "utf8/unchecked.h"

namespace mle {
void Window::init() {
    MLE_I("Initializing Window module.");

    SDL_SetHint(SDL_HINT_APP_ID, "MLE");

    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            MLE_UNREACHABLE_LOG("SDL_Init failed: {}", SDL_GetError());
        }
    }

    logDisplays();

    const char* driver = SDL_GetCurrentVideoDriver();
    if (driver) {
        MLE_I("Using video driver: {}", driver);
    } else {
        MLE_W("SDL_GetCurrentVideoDriver returned null.");
    }

    u32 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;

    window_ = SDL_CreateWindow("MLE APP", 800, 600, flags);
    if (!window_) {
        MLE_UNREACHABLE_LOG("SDL_CreateWindow failed: {}", SDL_GetError());
    }

    SDL_StartTextInput(window_);
    SDL_ShowWindow(window_);

    log();
}

void Window::shutdown() {
    MLE_I("Shutting down Window module.");

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    if (SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    SDL_Quit();
}

void Window::poolEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT: {
                ed_.dispatch(window::ev::Close{});
            } break;
            case SDL_EVENT_KEY_DOWN: {
                auto key = systemKeyToKey(event.key.key);
                UserInputManager::i().setPressed(key);
            } break;
            case SDL_EVENT_KEY_UP: {
                auto key = systemKeyToKey(event.key.key);
                UserInputManager::i().setReleased(key);
            } break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                auto key = systemMouseButtonToKey(event.button.button);
                UserInputManager::i().setPressed(key);
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                auto key = systemMouseButtonToKey(event.button.button);
                UserInputManager::i().setReleased(key);
            } break;
            case SDL_EVENT_MOUSE_MOTION: {
                UserInputManager::i().setCursorPos({event.motion.x, event.motion.y});
            } break;
            case SDL_EVENT_MOUSE_WHEEL: {
                UserInputManager::i().setScrollOffset(event.wheel.y);
            } break;
            case SDL_EVENT_TEXT_INPUT: {
                std::u32string u32str = toUtf32(std::string(event.text.text));
                for (auto cp : u32str) {
                    UserInputManager::i().pushChar(cp);
                }
            } break;
            case SDL_EVENT_WINDOW_MOUSE_ENTER: {
                UserInputManager::i().setMouseInsideWindow(true);
            } break;
            case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
                UserInputManager::i().setMouseInsideWindow(false);
            } break;
            case SDL_EVENT_WINDOW_RESIZED: {
                MLE_I("Window resized to {}x{}", event.window.data1, event.window.data2);
                ed_.dispatch(window::ev::Resize{.size = {event.window.data1, event.window.data2}});
            } break;
            default:
                MLE_NOOP;
                break;
        }
    }
}

VkSurfaceKHR Window::createSurface(VkInstance instance) const {
    MLE_ASSERT_LOG(window_, "Window not created.");
    VkSurfaceKHR ret = nullptr;
    if (!SDL_Vulkan_CreateSurface(window_, instance, nullptr, &ret)) {
        Core::i().unrecoverable("Failed to create Vulkan surface.");
    }
    return ret;
}

[[nodiscard]] std::vector<const char*> Window::getRequiredInstanceExtensions() {
    Uint32 count = 0;
    const char* const* names = SDL_Vulkan_GetInstanceExtensions(&count);
    if (!names || count == 0) {
        MLE_UNREACHABLE_LOG("SDL_Vulkan_GetInstanceExtensions returned no extensions. SDL error: {}", SDL_GetError());
    }
    std::vector<const char*> ret;
    ret.reserve(count);
    for (Uint32 i = 0; i < count; ++i) {
        /// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
        ret.push_back(names[i]);
    }
    return ret;
}

vec2i Window::getSize() const {
    MLE_ASSERT_LOG(window_, "Window not created.");
    int w = 0, h = 0;
    if (!SDL_GetWindowSizeInPixels(window_, &w, &h)) {
        MLE_UNREACHABLE_LOG("SDL_GetWindowSizeInPixels failed: {}", SDL_GetError());
    }
    return {w, h};
}

void Window::setTitle(const char* title) {
    MLE_ASSERT_LOG(window_, "Window not created.");
    MLE_T("Setting window title to '{}'", title);
    SDL_SetWindowTitle(window_, title);
}

void Window::show() {
    MLE_ASSERT_LOG(window_, "Window not created.");
    SDL_ShowWindow(window_);
}

void Window::hide() {
    MLE_ASSERT_LOG(window_, "Window not created.");
    SDL_HideWindow(window_);
}

void Window::setFullscreen(bool enable) const {
    if (!SDL_SetWindowFullscreen(window_, enable)) {
        MLE_UNREACHABLE_LOG("Failed to set window fullscreen mode: {}", SDL_GetError());
    }
}

void Window::setBorderlessFullscreen(bool enable) const {
    if (enable) {
        SDL_DisplayID display = SDL_GetDisplayForWindow(window_);
        SDL_Rect bounds;
        if (!SDL_GetDisplayBounds(display, &bounds)) {
            MLE_UNREACHABLE_LOG("Failed to get display bounds: {}", SDL_GetError());
        }

        SDL_SetWindowBordered(window_, false);
        SDL_SetWindowResizable(window_, false);
        SDL_SetWindowPosition(window_, bounds.x, bounds.y);
        SDL_SetWindowSize(window_, bounds.w, bounds.h);
    } else {
        SDL_SetWindowBordered(window_, true);
        SDL_SetWindowResizable(window_, true);
    }
}

[[nodiscard]] SDL_DisplayID Window::currentDisplay() const {
    return SDL_GetDisplayForWindow(window_);
}

void Window::moveToDisplay(SDL_DisplayID display, bool center, bool resizeToDisplay) const {
    SDL_Rect b{};
    if (!SDL_GetDisplayBounds(display, &b)) {
        MLE_UNREACHABLE_LOG("SDL_GetDisplayBounds failed: {}", SDL_GetError());
    }
    int win_w = 0, win_h = 0;
    if (!SDL_GetWindowSize(window_, &win_w, &win_h)) {
        MLE_UNREACHABLE_LOG("SDL_GetWindowSize failed: {}", SDL_GetError());
    }
    if (resizeToDisplay) {
        SDL_SetWindowBordered(window_, false);
        SDL_SetWindowResizable(window_, false);
        SDL_SetWindowPosition(window_, b.x, b.y);
        SDL_SetWindowSize(window_, b.w, b.h);
    } else if (center) {
        const int x = b.x + ((b.w - win_w) / 2);
        const int y = b.y + ((b.h - win_h) / 2);
        SDL_SetWindowPosition(window_, x, y);
    } else {
        SDL_SetWindowPosition(window_, b.x, b.y);
    }
}

std::vector<Window::DisplayInfo> Window::listDisplays() {
    int count = 0;
    SDL_DisplayID* ids = SDL_GetDisplays(&count);
    if (!ids || count <= 0) {
        MLE_UNREACHABLE_LOG("SDL_GetDisplays returned no displays. SDL error: {}", SDL_GetError());
    }
    std::vector<DisplayInfo> ret;
    ret.reserve(count);
    for (int i = 0; i < count; ++i) {
        DisplayInfo display;
        /// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
        display.id = ids[i];
        if (const char* name = SDL_GetDisplayName(display.id)) {
            display.name = name;
        }
        SDL_GetDisplayBounds(display.id, &display.bounds);
        display.contentScale = SDL_GetDisplayContentScale(display.id);
        ret.push_back(std::move(display));
    }
    SDL_free(ids);
    return ret;
}

void Window::log() const {
    u32 flags = SDL_GetWindowFlags(window_);
    int x = 0, y = 0, wpx = 0, hpx = 0;
    SDL_GetWindowPosition(window_, &x, &y);
    SDL_GetWindowSizeInPixels(window_, &wpx, &hpx);
    SDL_DisplayID dpy = currentDisplay();
    const auto* display_name = SDL_GetDisplayName(dpy);
    SDL_Rect b{};
    SDL_GetDisplayBounds(dpy, &b);
    MLE_I("Window info: pos:{},{}, size:{}x{}, flags:0x{:x}, display:{}({}) [{},{},{}x{}]", x, y, wpx, hpx, flags, display_name, (u32)dpy, b.x, b.y, b.w, b.h);
};

void Window::logDisplayInfo(SDL_DisplayID id) {
    const char* name = SDL_GetDisplayName(id);
    SDL_Rect bounds{};
    if (!SDL_GetDisplayBounds(id, &bounds)) {
        MLE_UNREACHABLE_LOG("SDL_GetDisplayBounds failed: {}", SDL_GetError());
    }
    float scale = SDL_GetDisplayContentScale(id);
    MLE_I("Display {} (ID: {}): name='{}', bounds=[{}, {}, {}x{}], contentScale={:.2f}", name ? name : "<unknown>", (u32)id, name ? name : "<unknown>",
          bounds.x, bounds.y, bounds.w, bounds.h, scale);
}

void Window::logDisplays() {
    auto displays = listDisplays();
    MLE_I("Found {} displays:", displays.size());
    for (const auto& d : displays) {
        MLE_I("  ID: {} Name: '{}' Bounds: [{}, {}, {}x{}] ContentScale: {:.2f}", (u32)d.id, d.name, d.bounds.x, d.bounds.y, d.bounds.w, d.bounds.h,
              d.contentScale);
    }
}

void Window::setPosition(int x, int y) const {
    MLE_ASSERT_LOG(window_, "Window not created.");
    SDL_SetWindowPosition(window_, x, y);
}

void Window::setSize(int w, int h) const {
    MLE_ASSERT_LOG(window_, "Window not created.");
    SDL_SetWindowSize(window_, w, h);
}

}  // namespace mle
