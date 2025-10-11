#pragma once

#include <map>
#include <thread>

#include "Types.h"
#include "mle/renderer/SyncManager.h"
#include "mle/utils/Utils.h"
#include "mle/window/Types.h"

// command buffers must be single threaded, so it is the user's responsibility to ensure that
// get and submit are called from the same thread

namespace mle {
class CommandBuffer {
  public:
    MLE_NO_COPY(CommandBuffer)

    CommandBuffer(CommandBuffer&& other) = default;
    CommandBuffer& operator=(CommandBuffer&& other) = default;

    ~CommandBuffer() = default;

    [[nodiscard]] vk::CommandBuffer get() const { return o_; }
    [[nodiscard]] bool isPrimary() const { return primary_; }
    [[nodiscard]] auto tid() const { return tid_; }
    [[nodiscard]] auto queueDataIdx() const { return queue_data_idx_; }

    vk::CommandBuffer operator()() const { return o_; }

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

    ResetCommandPool(ResetCommandPool&& other) noexcept :
        o_(other.o_),
        queue_data_idx_(other.queue_data_idx_),
        available_primary_buffers_(std::move(other.available_primary_buffers_)),
        available_secondary_buffers_(std::move(other.available_secondary_buffers_)) {
        other.o_ = nullptr;
    }

    ResetCommandPool& operator=(ResetCommandPool&& other) noexcept {
        if (this != &other) {
            o_ = other.o_;
            queue_data_idx_ = other.queue_data_idx_;
            available_primary_buffers_ = std::move(other.available_primary_buffers_);
            available_secondary_buffers_ = std::move(other.available_secondary_buffers_);
            other.o_ = nullptr;
        }
        return *this;
    }

    ~ResetCommandPool() = default;

    void reset();

    CommandBuffer getPrimary();
    CommandBuffer getSecondary();

    void submit(CommandBuffer&& cmd, vk::SubmitInfo2 submit_info);
    void reclaim(CommandBuffer&& cmd);

  private:
    friend RendererCommandManager;
    ResetCommandPool(vk::CommandPool pool, QueueDataIdx queue_data_idx) :
        o_(pool),
        queue_data_idx_(queue_data_idx) {}

  private:
    vk::CommandPool o_;
    QueueDataIdx queue_data_idx_ = INVALID_QUEUE;
    std::vector<vk::CommandBuffer> available_primary_buffers_;
    std::vector<vk::CommandBuffer> available_secondary_buffers_;
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

    ResetCommandPool makeResetableCommandPool(GCmdType type);

    Fence submit(QueueDataIdx queue_data_idx, vk::SubmitInfo2 submit_info);
    Fence submit(GCmdType type, vk::SubmitInfo2 submit_info) { return submit(queueDataIdx(type), submit_info); }

    CommandBuffer getOTS(QueueDataIdx queue_data_idx);
    CommandBuffer getOTS(GCmdType type) { return getOTS(queueDataIdx(type)); }
    void submitOTSWait(CommandBuffer&& cmd, vk::SubmitInfo2 submit_info = {});
    void submitOTSAsync(CommandBuffer&& cmd, vk::SubmitInfo2 submit_info = {}, std::move_only_function<void(void)>&& callback = {});
    void reclaimOTS(CommandBuffer&& cmd);

    [[nodiscard]] QueueDataIdx queueDataIdx(GCmdType type) const { return cmd_type_to_qdidx_.at(as<u32>(type)); }
    [[nodiscard]] QueueDataIdx queueFamilyIdx(GCmdType type) const { return queueData(type).family_index; }
    [[nodiscard]] QueueDataIdx queueFamilyIdx(QueueDataIdx queue_data_idx) const { return queueData(queue_data_idx).family_index; }

  private:
    friend Renderer;
    RendererCommandManager() = default;
    void init();
    void shutdown();

    QueueData& queueData(QueueDataIdx queue_data_idx) { return queue_data_.at(queue_data_idx); }
    QueueData& queueData(GCmdType type) { return queue_data_.at(queueDataIdx(type)); }
    [[nodiscard]] const QueueData& queueData(GCmdType type) const { return queue_data_.at(queueDataIdx(type)); }
    [[nodiscard]] const QueueData& queueData(QueueDataIdx queue_data_idx) const { return queue_data_.at(queue_data_idx); }

  private:
    std::array<QueueDataIdx, 3> cmd_type_to_qdidx_{INVALID_QUEUE, INVALID_QUEUE, INVALID_QUEUE};
    std::array<QueueData, 3> queue_data_{};
};

}  // namespace mle
