#include "FencePool.h"

#include "mle/common/Assert.h"
#include "mle/core/Core.h"
#include "mle/renderer/Renderer.h"

namespace mle::renderer::detail {
void FencePool::reset() {
    MLE_D("Resetting FencePool");

    std::scoped_lock lock(available_fences_mutex_);

    for (auto& fence : available_fences_) {
        getDevice().destroyFence(fence);
    }
    available_fences_.clear();

    MLE_ASSERT_LOG(async_fences_.empty(), "There are still async fences in the pool! TODO: Check if this is ok.");
    async_fences_.clear();

    MLE_D("FencePool reset complete");
}

void FencePool::update() {
    std::scoped_lock lock(available_fences_mutex_);

    for (auto it = async_fences_.begin(); it != async_fences_.end();) {
        auto fence = it->first;
        if (getDevice().getFenceStatus(fence) == vk::Result::eSuccess) {
            if (it->second) {
                it->second();
            }
            getDevice().resetFences(fence);
            available_fences_.push_back(fence);
            it = async_fences_.erase(it);
        } else {
            ++it;
        }
    }
}

void FencePool::release(vk::Fence fence) {
    std::scoped_lock lock(available_fences_mutex_);

    MLE_ASSERT(fence);

    getDevice().resetFences(fence);

    available_fences_.push_back(fence);
}

vk::Fence FencePool::get() {
    std::scoped_lock lock(available_fences_mutex_);

    if (available_fences_.empty()) {
        vk::FenceCreateInfo fence_ci;
        auto create_r = getDevice().createFence(fence_ci);
        if (create_r.result != vk::Result::eSuccess) {
            core::unrecoverable("Failed to create fence! Error code: {}", create_r.result);
        }
        return create_r.value;
    }

    auto ret = available_fences_.back();
    available_fences_.pop_back();
    return ret;
}

void FencePool::waitAsync(vk::Fence fence, std::move_only_function<void(void)>&& callback) {
    std::scoped_lock lock(available_fences_mutex_);
    async_fences_.emplace_back(fence, std::move(callback));
}
}  // namespace mle::renderer::detail
