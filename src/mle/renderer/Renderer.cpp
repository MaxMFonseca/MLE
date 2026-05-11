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
    model_cache_.init();
    skeleton_cache_.init();
    animation_cache_.init();

    MLE_I("Renderer initialized successfully");
}

void Renderer::shutdown() {
    auto device = vk_ctx_.getDevice();

    if (!device) {
        return;
    }

    std::ignore = device.waitIdle();

    animation_cache_.shutdown();
    skeleton_cache_.shutdown();
    model_cache_.shutdown();
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
