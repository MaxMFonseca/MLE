#pragma once

#include "SDL3/SDL_video.h"
#include "SDL3/SDL_vulkan.h"
#include "mle/utils/Utils.h"
#include "mle/window/Events.h"

namespace mle {
class Window final {
    MLE_SINGLETON(Window)
  public:
    struct Config {};

    struct DisplayInfo {
        SDL_DisplayID id{};
        std::string name;
        SDL_Rect bounds{};
        float contentScale{};
    };

  public:
    void init();
    void shutdown();

    void poolEvents();

    [[nodiscard]] vec2i queryFramebufferSize() const;

    void setTitle(const char* title);
    void show();
    void hide();
    void setFullscreen(bool enable) const;
    void setBorderlessFullscreen(bool enable) const;
    void moveToDisplay(SDL_DisplayID display, bool center = true, bool resizeToDisplay = false) const;
    [[nodiscard]] SDL_DisplayID currentDisplay() const;

    void setPosition(int x, int y) const;
    void setSize(int w, int h) const;

    [[nodiscard]] VkSurfaceKHR createSurface(VkInstance instance) const;

    void log() const;

    [[nodiscard]] static std::vector<const char*> getRequiredInstanceExtensions();

    [[nodiscard]] static std::vector<DisplayInfo> listDisplays();
    static void logDisplayInfo(SDL_DisplayID id);
    static void logDisplays();

    [[nodiscard]] SDL_Window* getSDLWindow() const { return window_; }
    [[nodiscard]] auto& getED() { return ed_; }

  private:
    SDL_Window* window_ = nullptr;
    window::ev::Dispatcher ed_;
};
}  // namespace mle
