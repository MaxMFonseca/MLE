#include "Renderer.h"

#include "VkCtx.h"

namespace mle {
void Renderer::init() {
    vk_ctx_.init();
    command_manager_.init();
    sync_manager_.init();
}

void Renderer::shutdown() {
    sync_manager_.shutdown();
    command_manager_.shutdown();
    vk_ctx_.shutdown();
}
}  // namespace mle
