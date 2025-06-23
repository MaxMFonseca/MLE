#pragma once

#include <functional>

#include "Types.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
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

namespace detail {
FontCache& getFontCache();
}  // namespace detail
}  // namespace mle::ui
