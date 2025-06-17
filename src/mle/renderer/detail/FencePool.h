#pragma once

#include "../Types.h"

namespace mle::renderer::detail {
class FencePool final {
  public:
    FencePool(const FencePool&) = delete;
    FencePool& operator=(const FencePool&) = delete;
    FencePool(FencePool&&) = delete;
    FencePool& operator=(FencePool&&) = delete;

    FencePool() = default;
    ~FencePool() = default;

    void reset();

    void update();

    vk::Fence get();
    void release(vk::Fence fence);

    void waitAsync(vk::Fence fence, std::function<void(void)>&& callback = {});

  private:
    std::mutex available_fences_mutex_;
    std::vector<vk::Fence> available_fences_;

    std::vector<std::pair<vk::Fence, std::function<void(void)>>> async_fences_;
};
}  // namespace mle::renderer::detail
