#include "FencePool.h"

#include "mle/common/Assert.h"
#include "mle/core/Core.h"
#include "mle/renderer/Renderer.h"

namespace mle::renderer::detail {
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
        fence_ci.flags = vk::FenceCreateFlagBits::eSignaled;
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
}  // namespace mle::renderer::detail
