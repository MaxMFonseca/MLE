#pragma once

#include <array>

#include "../Types.h"
#include "mle/common/Utils.h"

namespace mle::renderer::detail {
class CommandPoolManager {
  public:
    CommandPoolManager(const CommandPoolManager&) = delete;
    CommandPoolManager& operator=(const CommandPoolManager&) = delete;
    CommandPoolManager(CommandPoolManager&&) = delete;
    CommandPoolManager& operator=(CommandPoolManager&&) = delete;

    CommandPoolManager() = default;
    ~CommandPoolManager() = default;

    void init();
    void reset();

    CmdPoolData acquire(CmdType type);
    void release(CmdType type, CmdPoolData&& pool);

    Fence submit(CmdType type, const vk::SubmitInfo2& info);

  private:
    auto& getData(CmdType type) { return data_.at(as<usize>(type)); }
    auto& gData() { return getData(CmdType::GRAPHICS); }
    auto& tData() { return getData(CmdType::TRANSFER); }
    auto& cData() { return getData(CmdType::COMPUTE); }

  private:
    struct TypeData {
        std::vector<CmdPoolData> available_pools;
        usize queue_index;
        vk::Queue queue;
        std::mutex queue_mutex;
    };
    std::array<TypeData, 3> data_;
};
}  // namespace mle::renderer::detail
