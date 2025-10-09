#include "Renderer.h"

#include "VkCtx.h"

namespace mle {
void Renderer::init() {
    VkCtx::i().init();
}

void Renderer::shutdown() {
    VkCtx::i().shutdown();
}
}  // namespace mle
