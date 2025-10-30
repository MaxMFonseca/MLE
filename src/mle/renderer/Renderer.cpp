#include "Renderer.h"

#include "VkCtx.h"

namespace mle {
void Renderer::init() {
    MLE_I("Initializing Renderer");

    vk_ctx_.init();
    command_manager_.init();
    sync_manager_.init();
    frame_renderer_.init();
    shader_cache_.init();
    pipeline_cache_.init();
    texture_cache_.init();
    font_cache_.init();

    MLE_I("Renderer initialized successfully");
}

void Renderer::shutdown() {
    std::ignore = vk_ctx_.getDevice().waitIdle();

    font_cache_.shutdown();
    texture_cache_.shutdown();
    pipeline_cache_.shutdown();
    shader_cache_.shutdown();
    sync_manager_.shutdown();
    command_manager_.shutdown();
    frame_renderer_.shutdown();
    vk_ctx_.shutdown();
}
}  // namespace mle
