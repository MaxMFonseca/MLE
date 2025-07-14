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

void namedElementCreated(const std::string& name, entt::entity element);
void listenNamedElementCreated(const std::string& name, std::function<void(entt::entity)>&& callback);

entt::entity getElement(const std::string& name);

ID listenUIEvent(const std::string& name, std::move_only_function<void(const sol::object& o)>&& func, bool once = false);
void removeUIEventListener(const std::string& name, ID id);
void dispatchUIEvent(const std::string& name, const sol::object& o);

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
