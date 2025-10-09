#pragma once

#include "CommandManager.h"
#include "SyncManager.h"
#include "mle/renderer/VkCtx.h"
#include "mle/utils/Utils.h"

namespace mle {
class Renderer {
    MLE_SINGLETON(Renderer)

  public:
    void init();
    void shutdown();

    auto& vkCtx() { return vk_ctx_; }
    auto& vk() { return vk_ctx_; }
    auto& commandManager() { return command_manager_; }
    auto& cmdMgr() { return command_manager_; }
    auto& syncManager() { return sync_manager_; }
    auto& syncMgr() { return sync_manager_; }

    template <class T>
    void destroy(T o) {
        vk_ctx_.destroy(o);
    }

  private:
    VkCtx vk_ctx_;
    RendererCommandManager command_manager_;
    RendererSyncManager sync_manager_;
};
}  // namespace mle
