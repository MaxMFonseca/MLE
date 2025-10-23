#pragma once

#include "CommandManager.h"
#include "FrameRenderer.h"
#include "PipelineCache.h"
#include "ShaderCache.h"
#include "SyncManager.h"
#include "VkCtx.h"
#include "mle/renderer/FontCache.h"
#include "mle/renderer/TextureCache.h"
#include "mle/utils/Utils.h"

namespace mle {
class Renderer {
    MLE_SINGLETON(Renderer)

  public:
    void init();
    void shutdown();

    auto& vkCtx() { return vk_ctx_; }
    auto& vk() { return vk_ctx_; }
    vk::Device vkDevice() { return vk_ctx_.getDevice(); }
    auto& commandManager() { return command_manager_; }
    auto& cmdMgr() { return command_manager_; }
    auto& syncManager() { return sync_manager_; }
    auto& syncMgr() { return sync_manager_; }
    auto& shaderCache() { return shader_cache_; }
    auto& pipelineCache() { return pipeline_cache_; }
    auto& frameRenderer() { return frame_renderer_; }
    auto& textureCache() { return texture_cache_; }
    auto& fontCache() { return font_cache_; }

    template <class T>
    void destroy(T o) {
        vk_ctx_.destroy(o);
    }

  private:
    VkCtx vk_ctx_;
    RendererCommandManager command_manager_;
    RendererSyncManager sync_manager_;
    ShaderCache shader_cache_;
    PipelineCache pipeline_cache_;
    FrameRenderer frame_renderer_;
    TextureCache texture_cache_;
    FontCache font_cache_;
};
}  // namespace mle
