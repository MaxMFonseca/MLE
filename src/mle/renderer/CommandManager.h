#pragma once

#include <map>
#include <thread>

#include "Types.h"
#include "mle/renderer/SyncManager.h"
#include "mle/utils/Utils.h"

// command buffers must be single threaded, so it is the user's responsibility to ensure that
// get and submit are called from the same thread
// FIXME: REWORK THIS WHOLE THING!!!!!!!!
// This is extremely jank and bad and needs to be redone from the ground up

namespace mle {
class CommandBuffer {
  public:
    MLE_NO_COPY(CommandBuffer)

    CommandBuffer(CommandBuffer&& other);
    CommandBuffer& operator=(CommandBuffer&& other);

    CommandBuffer() = default;
    ~CommandBuffer() {
        MLE_ASSERT_LOG(!o_, "Destroying a CommandBuffer that still owns a Vulkan command buffer. Did you forget to submit/reclaim/invalidate(resetpool) it?");
    }

    [[nodiscard]] vk::CommandBuffer get() const { return o_; }
    [[nodiscard]] bool isPrimary() const { return primary_; }
    [[nodiscard]] auto tid() const { return tid_; }
    [[nodiscard]] auto queueDataIdx() const { return queue_data_idx_; }
    void invalidate() { o_ = nullptr; }  // TODO: idk man

    vk::CommandBuffer operator()() const {
        MLE_ASSERT_LOG(o_, "Attempting to use a null command buffer.");
        return o_;
    }

  private:
    friend RendererCommandManager;
    friend ResetCommandPool;
    CommandBuffer(vk::CommandBuffer buf, QueueDataIdx queue_data_idx, bool primary) :
        o_(buf),
        tid_(std::this_thread::get_id()),
        queue_data_idx_(queue_data_idx),
        primary_(primary) {}

  private:
    vk::CommandBuffer o_;
    std::thread::id tid_;
    QueueDataIdx queue_data_idx_ = INVALID_QUEUE;
    bool primary_{true};
};

class ResetCommandPool {
  public:
    MLE_NO_COPY(ResetCommandPool)

    ResetCommandPool(ResetCommandPool&& other);
    ResetCommandPool& operator=(ResetCommandPool&& other);

    ResetCommandPool() = default;
    ~ResetCommandPool() { shutdown(); }

    void shutdown();

    void reset();

    CommandBuffer getPrimary();
    CommandBuffer getSecondary();

    void submit(CommandBuffer&& cmd, vk::SubmitInfo2 submit_info, vk::Fence fence) const;
    void submitWait(CommandBuffer&& cmd, vk::SubmitInfo2 submit_info) const;

  private:
    friend RendererCommandManager;
    ResetCommandPool(vk::CommandPool pool, QueueDataIdx queue_data_idx) :
        o_(pool),
        queue_data_idx_(queue_data_idx) {}

  private:
    vk::CommandPool o_{};
    QueueDataIdx queue_data_idx_ = INVALID_QUEUE;
    std::vector<vk::CommandBuffer> primary_buffers_;
    std::vector<vk::CommandBuffer> secondary_buffers_;
    usize primary_index_{0};
    usize secondary_index_{0};
};

class RendererCommandManager {
  private:
    struct QueueData {
        struct OTSCommandPoolData {
            vk::CommandPool pool;
            std::vector<vk::CommandBuffer> available_primary_buffers;
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

    std::unique_lock<std::mutex> waitIdle(GCmdType type);

    [[nodiscard]] ResetCommandPool createResetCommandPool(GCmdType type);

    void submit(QueueDataIdx queue_data_idx, vk::SubmitInfo2 submit_info, vk::Fence fence);
    [[nodiscard]] Fence submit(QueueDataIdx queue_data_idx, vk::SubmitInfo2 submit_info);
    [[nodiscard]] Fence submit(GCmdType type, vk::SubmitInfo2 submit_info) { return submit(queueDataIdx(type), submit_info); }

    [[nodiscard]] CommandBuffer getOTS(QueueDataIdx queue_data_idx);
    [[nodiscard]] CommandBuffer getOTS(GCmdType type) { return getOTS(queueDataIdx(type)); }
    void submitOTSWait(CommandBuffer&& cmd, vk::SubmitInfo2 submit_info = {});
    void submitOTSAsync(CommandBuffer&& cmd, vk::SubmitInfo2 submit_info = {}, std::move_only_function<void(void)>&& callback = {});
    void reclaimOTS(CommandBuffer&& cmd);

    [[nodiscard]] vk::Result submitPresent(const vk::PresentInfoKHR& present_info);

    [[nodiscard]] QueueDataIdx queueDataIdx(GCmdType type) const { return cmd_type_to_qdidx_.at(as<u32>(type)); }
    [[nodiscard]] QueueDataIdx queueFamilyIdx(GCmdType type) const { return queueData(type).family_index; }
    [[nodiscard]] QueueDataIdx queueFamilyIdx(QueueDataIdx queue_data_idx) const { return queueData(queue_data_idx).family_index; }

  private:
    friend Renderer;
    RendererCommandManager() = default;
    void init();
    void shutdown();

    [[nodiscard]] QueueData& queueData(QueueDataIdx queue_data_idx) { return queue_data_.at(queue_data_idx); }
    [[nodiscard]] QueueData& queueData(GCmdType type) { return queue_data_.at(queueDataIdx(type)); }
    [[nodiscard]] const QueueData& queueData(GCmdType type) const { return queue_data_.at(queueDataIdx(type)); }
    [[nodiscard]] const QueueData& queueData(QueueDataIdx queue_data_idx) const { return queue_data_.at(queue_data_idx); }

  private:
    std::array<QueueDataIdx, 3> cmd_type_to_qdidx_{INVALID_QUEUE, INVALID_QUEUE, INVALID_QUEUE};
    std::array<QueueData, 3> queue_data_{};
};

}  // namespace mle
