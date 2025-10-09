#pragma once

#include <map>
#include <thread>

#include "Types.h"
#include "mle/renderer/SyncManager.h"
#include "mle/utils/Utils.h"

// command buffers must be single threaded, so it is the user's responsibility to ensure that
// get and submit are called from the same thread

namespace mle {
class RendererCommandManager {
  private:
    struct QueueData {
        struct OTSCommandPoolData {
            vk::CommandPool pool;
            std::vector<vk::CommandBuffer> available_buffers;
        };
        std::mutex pool_map_mutex;
        std::map<std::thread::id, OTSCommandPoolData> thread_ots_pool_map;

        std::mutex submit_mutex;

        vk::Queue queue;
        usize family_index;
    };

  public:
    MLE_NO_COPY_MOVE(RendererCommandManager);

    ~RendererCommandManager() = default;

    vk::CommandPool makeCmdPool(GCmdType type, vk::CommandPoolCreateFlags flags = {});
    Fence submit(GCmdType type, vk::SubmitInfo2 submit_info);

    vk::CommandBuffer getOTS(GCmdType type);
    void submitOTSWait(GCmdType type, vk::SubmitInfo2 submit_info);
    void submitOTSAsync(GCmdType type, vk::SubmitInfo2 submit_info, std::move_only_function<void(void)>&& callback = {});

  private:
    friend Renderer;
    RendererCommandManager() = default;
    void init();
    void shutdown();

    QueueData& queueData(GCmdType type);

    void reclaimOTS(vk::CommandBuffer cmd, GCmdType type, std::thread::id tid);

  private:
    std::array<usize, 3> cmd_type_to_index_{max<usize>(), max<usize>(), max<usize>()};
    std::array<QueueData, 3> queue_data_{};

    std::vector<vk::CommandPool> external_pools_;
};

}  // namespace mle
