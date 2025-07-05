#pragma once

#include <list>

#include "Types.h"
#include "mle/common/Consts.h"
#include "mle/common/Result.h"
#include "mle/renderer/Fence.h"

namespace mle::renderer {
class CommandPool final {
  public:
    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;
    CommandPool& operator=(CommandPool&&) = delete;

    CommandPool(CommandPool&&);

    CommandPool() = default;
    ~CommandPool();

    void init(CmdType type);
    void reset();

    vk::CommandBuffer getCmd();

    void submitWait(vk::CommandBuffer cmd);
    void submitAsync(vk::CommandBuffer cmd, std::move_only_function<void(void)>&& callback);
    void submitAsync(vk::SubmitInfo2 submit_info, std::move_only_function<void(void)>&& callback = {});

  private:
    void releaseCmdBuffer(vk::CommandBuffer cmd);

  private:
    CmdType type_ = CmdType::INVALID;
    CmdPoolData o_;
    std::mutex mutex_;
    usize cmd_buffer_count_ = 0;
};
}  // namespace mle::renderer
