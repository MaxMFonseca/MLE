#include "CommandPoolManager.h"

#include <functional>

#include "../CommandPool.h"
#include "../Fence.h"
#include "VkContext.h"
#include "mle/common/Assert.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"

namespace mle::renderer::detail {
void CommandPoolManager::reset() {
    MLE_D("Reseting the CommandPoolManager");

    for (auto& type_data : data_) {
        for (auto& pool : type_data.available_pools) {
            getDevice().destroyCommandPool(pool.o);
        }
        type_data.available_pools.clear();
    }
}

void CommandPoolManager::init() {
    MLE_D("Initializing CommandPoolManager");

    auto queues = detail::getVk().getDedicatedQueues();

    gData().queue_index = detail::getVk().getQueueIndex(CmdType::GRAPHICS);
    gData().queue = queues.at(as<usize>(CmdType::GRAPHICS));

    tData().queue_index = detail::getVk().getQueueIndex(CmdType::TRANSFER);
    tData().queue = queues.at(as<usize>(CmdType::TRANSFER));

    // cData().queue_index = detail::getVk().getQueueIndex(CmdType::COMPUTE);
    // cData().queue = queues.at(as<usize>(CmdType::COMPUTE));
}
CommandPoolManager::TypeData& CommandPoolManager::getData(CmdType type) {
    if (type == CmdType::COMPUTE) {
        MLE_W("No dedicated compute queue, using graphics queue instead.");
        // TODO: this? maybe?
        type = CmdType::GRAPHICS;
    }

    return data_.at(as<usize>(type));
}

CmdPoolData CommandPoolManager::acquirePool(CmdType type) {
    auto& data = getData(type);

    if (data.available_pools.empty()) {
        MLE_D("Creating new command pool for type: {}", type);
        vk::CommandPoolCreateInfo pool_ci{};
        pool_ci.queueFamilyIndex = data.queue_index;
        pool_ci.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

        auto c_r = getDevice().createCommandPool(pool_ci);
        check(c_r.result);
        return {.o = c_r.value, .available_buffers = {}};
    }

    auto ret = std::move(data.available_pools.back());
    data.available_pools.pop_back();
    return ret;
}

void CommandPoolManager::release(CmdType type, CmdPoolData&& pool) {
    auto& data = getData(type);

    auto reset_result = getDevice().resetCommandPool(pool.o);
    check(reset_result);

    data.available_pools.emplace_back(std::move(pool));
}

Fence CommandPoolManager::submit(CmdType type, const vk::SubmitInfo2& info) {
    auto& data = getData(type);
    auto fence = Fence::create();
    data.queue_mutex.lock();
    check(data.queue.submit2(info, fence.get()));
    data.queue_mutex.unlock();
    return fence;
}
}  // namespace mle::renderer::detail
