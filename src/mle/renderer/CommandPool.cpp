#include "CommandPool.h"

#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "Fence.h"
#include "Renderer.h"
#include "detail/CommandPoolManager.h"
#include "mle/common/Assert.h"
#include "mle/renderer/Utils.h"

namespace mle::renderer {
CommandPool::CommandPool(CommandPool&& other) :
    type_(other.type_),
    o_(std::move(other.o_)) {
    other.type_ = CmdType::INVALID;
}

CommandPool::~CommandPool() {
    if (type_ == CmdType::INVALID) {
        return;
    }

    detail::getCommandPoolManager().release(type_, std::move(o_));
}

CommandPool CommandPool::create(CmdType type) {
    CommandPool ret;
    ret.init(type);
    return ret;
}

void CommandPool::init(CmdType type) {
    type_ = type;
    o_ = detail::getCommandPoolManager().acquire(type);
}

Fence CommandPool::submit(vk::CommandBuffer cmd) {
    MLE_ASSERT_LOG(isPrimary(cmd), "Command buffer is not primary");

    vk::CommandBufferSubmitInfo cmd_info;
    cmd_info.setCommandBuffer(cmd);

    vk::SubmitInfo2 info;
    info.setCommandBufferInfos(cmd_info);

    return detail::getCommandPoolManager().submit(type_, info);
}

bool CommandPool::isPrimary(vk::CommandBuffer cmd) {
    auto it = std::ranges::find_if(o_.buffers, [&](const auto& buffer) { return buffer == cmd; });
    return it != o_.buffers.end();
}

bool CommandPool::isSecondary(vk::CommandBuffer cmd) {
    auto it = std::ranges::find_if(o_.secondary_buffers, [&](const auto& buffer) { return buffer == cmd; });
    return it != o_.secondary_buffers.end();
}

bool CommandPool::isValid(vk::CommandBuffer cmd) {
    return isPrimary(cmd) || isSecondary(cmd);
}

vk::CommandBuffer CommandPool::getCmdBuffer() {
    if (p_counter_ < o_.buffers.size()) {
        return o_.buffers[p_counter_++];
    }

    vk::CommandBufferAllocateInfo alloc_info;
    alloc_info.setCommandPool(o_.o).setLevel(vk::CommandBufferLevel::ePrimary).setCommandBufferCount(1);
    auto alloc_r = detail::getDevice().allocateCommandBuffers(alloc_info);
    check(alloc_r.result);
    o_.buffers.push_back(alloc_r.value[0]);
    return o_.buffers[p_counter_++];
}

vk::CommandBuffer CommandPool::getCmdBufferSecondary() {
    if (s_counter_ < o_.secondary_buffers.size()) {
        return o_.secondary_buffers[s_counter_++];
    }

    vk::CommandBufferAllocateInfo alloc_info;
    alloc_info.setCommandPool(o_.o).setLevel(vk::CommandBufferLevel::eSecondary).setCommandBufferCount(1);
    auto alloc_r = detail::getDevice().allocateCommandBuffers(alloc_info);
    o_.secondary_buffers.push_back(alloc_r.value[0]);
    return o_.secondary_buffers[s_counter_++];
}
}  // namespace mle::renderer
