#pragma once

#include "Types.h"

namespace mle {

class GrowDescriptorPool final {
  public:
    struct CreateInfo {
        u32 max_set_per_pool = 1024;
        bool free_individual_sets = true;
        bool update_after_bind = false;

        std::vector<vk::DescriptorPoolSize> sizes;
        vk::DescriptorSetLayoutCreateInfo set_ci{};
    };

    struct Pool {
        vk::DescriptorPool pool;
        u32 allocated_sets = 0;
    };

  public:
    MLE_NO_COPY_MOVE(GrowDescriptorPool);

    GrowDescriptorPool() = default;
    ~GrowDescriptorPool();

    void init(CreateInfo ci);

    vk::DescriptorSet allocate(vk::DescriptorSetLayout layout);
    std::vector<vk::DescriptorSet> allocateMany(vk::DescriptorSetLayout layout, usize count);
    void resetAll();

  private:
    Pool& addPool();

  private:
    std::vector<Pool> pools_;
    vk::DescriptorPoolCreateInfo pool_ci_{};
};
}  // namespace mle
