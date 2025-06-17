/*
 * @file
 * @brief CommandPoolManager class declaration
 */
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

    CmdPoolData acquirePool(CmdType type);
    void release(CmdType type, CmdPoolData&& pool);

    [[nodiscard]] Fence submit(CmdType type, const vk::SubmitInfo2& info);

  private:
    struct TypeData {
        std::vector<CmdPoolData> available_pools;
        usize queue_index;
        vk::Queue queue;
        std::mutex queue_mutex;
    };
    TypeData& getData(CmdType type);

    auto& gData() { return getData(CmdType::GRAPHICS); }
    auto& tData() { return getData(CmdType::TRANSFER); }
    // auto& cData() { return getData(CmdType::COMPUTE); }

  private:
    std::array<TypeData, 2> data_;
};
}  // namespace mle::renderer::detail
