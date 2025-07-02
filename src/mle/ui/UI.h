#pragma once

#include <functional>

#include "Types.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
#include "mle/ui/Events.h"
#include "mle/ui/detail/FontCache.h"
#include "mle/ui/element/Base.h"

namespace mle::ui {
void init();
void shutdown();
void update();
renderer::ImageRef render();
entt::registry& getRegistry();
FontRef getFont(const std::string& font_name = "");

vk::CommandBuffer getPreRenderCmdBuffer();
void addJobToQueue(vk::CommandBuffer cmd);
vec2u getRootSize();

void setNextRoot(const sol::table& next_root);

// TODO: create a wrapper
void dispatchEvent(const std::string& event_name);
ID addListener(const std::string& event_name, std::function<void()>&& callback);
void removeListener(const std::string& event_name, ID id);

entt::entity getElement(const std::string& name);

namespace detail {
ED& getED();
}  // namespace detail

template <typename Event>
ED::ListenerHnd<Event> listen(std::function<void(const Event&)>&& callback) {
    return detail::getED().makeEventListener(std::move(callback));
}

template <typename Event>
void dispatch(const Event& event) {
    detail::getED().dispatch(event);
}
}  // namespace mle::ui
