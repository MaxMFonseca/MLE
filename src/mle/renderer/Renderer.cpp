#include "Renderer.h"

#include "VkCtx.h"

namespace mle {
void Renderer::init() {
    vk_ctx_.init();
    command_manager_.init();
    sync_manager_.init();
    frame_renderer_.init();
}

void Renderer::shutdown() {
    frame_renderer_.shutdown();
    pipeline_cache_.shutdown();
    shader_cache_.shutdown();
    sync_manager_.shutdown();
    command_manager_.shutdown();
    vk_ctx_.shutdown();
}
}  // namespace mle
