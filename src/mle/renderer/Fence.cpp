#include "Fence.h"

#include "Renderer.h"
#include "detail/FencePool.h"
#include "mle/common/Assert.h"
#include "mle/common/Logger.h"
#include "mle/renderer/Utils.h"

namespace mle::renderer {
Fence Fence::create() {
    Fence ret;

    ret.init();

    return ret;
}

void Fence::init() {
    MLE_ASSERT(!o_);

    o_ = detail::getFencePool().get();
}

Fence::Fence(Fence&& other) :
    o_(other.o_) {
    other.o_ = nullptr;
}

Fence::~Fence() {
    if (!o_) {
        return;
    }

    detail::getFencePool().release(o_);
}

void Fence::wait(u64 timeout_ns) const {
    auto result = detail::getDevice().waitForFences(o_, vk::True, timeout_ns);
    check(result);
    if (result == vk::Result::eTimeout) {
        core::unrecoverable("Vulkan fence wait timed out after {} ns", timeout_ns);
    }
}

void Fence::waitAsync(std::function<void(void)>&& callback) {
    detail::getFencePool().waitAsync(o_, std::move(callback));
    o_ = nullptr;
}
}  // namespace mle::renderer
