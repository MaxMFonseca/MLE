#pragma once

#include <list>

#include "Types.h"
#include "mle/common/Consts.h"
#include "mle/common/Result.h"

namespace mle::renderer {
class CommandPool final {
  public:
    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;
    CommandPool& operator=(CommandPool&&) = delete;

    CommandPool(CommandPool&&);

    ~CommandPool();

    static CommandPool create(CmdType type);
    static CommandPoolHnd createHnd(CmdType type);

    vk::CommandBuffer getCmdBuffer();
    vk::CommandBuffer getCmdBufferSecondary();

    Fence submit(vk::CommandBuffer cmd);

  private:
    CommandPool() = default;

    void init(CmdType type);

    bool isPrimary(vk::CommandBuffer cmd);
    bool isSecondary(vk::CommandBuffer cmd);
    inline bool isValid(vk::CommandBuffer cmd);

  private:
    CmdType type_ = CmdType::INVALID;
    CmdPoolData o_;
    usize p_counter_ = 0, s_counter_ = 0;
};
}  // namespace mle::renderer
