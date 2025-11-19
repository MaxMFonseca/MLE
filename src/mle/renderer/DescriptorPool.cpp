#include "DescriptorPool.h"

#include <spdlog/fmt/ranges.h>

#include "Renderer.h"
#include "vulkan/vulkan.hpp"

namespace mle {
namespace {
Expected<vk::DescriptorSet> tryAllocate(vk::DescriptorSetLayout layout, GrowDescriptorPool::Pool& pool) {
    vk::DescriptorSetAllocateInfo ai{};
    ai.descriptorPool = pool.pool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts = &layout;

    auto alloc_r = Renderer::i().vkDevice().allocateDescriptorSets(ai);

    if (alloc_r.result == vk::Result::eSuccess) {
        return alloc_r.value.at(0);
    }

    if (alloc_r.result == vk::Result::eErrorOutOfPoolMemory || alloc_r.result == vk::Result::eErrorFragmentedPool) {
        return std::unexpected(Result::ALLOCATION_FAILED);
    }

    Core::i().unrecoverable("Failed to allocate descriptor set: {}", as<vk::Result>(alloc_r.result));
    MLE_UNREACHABLE;
}
}  // namespace

vk::DescriptorSet GrowDescriptorPool::allocate(vk::DescriptorSetLayout layout) {
    for (auto& p : pools_) {
        auto ds = tryAllocate(layout, p);
        if (ds.has_value()) {
            return ds.value();
        }
    }

    auto& new_pool = addPool();

    auto ds = tryAllocate(layout, new_pool);
    if (ds.has_value()) {
        return ds.value();
    }

    Core::i().unrecoverable("Failed to allocate descriptor set even after adding a new pool.");
    MLE_UNREACHABLE;
}

void GrowDescriptorPool::init(CreateInfo ci) {
    if (ci.set_ci.bindingCount > 0) {
        MLE_ASSERT_LOG(ci.sizes.empty(), "Either provide sizes or a set create info with bindings, not both.");
        for (const auto& b : std::span(ci.set_ci.pBindings, ci.set_ci.bindingCount)) {
            bool found = false;
            for (auto& s : ci.sizes) {
                if (s.type == b.descriptorType) {
                    s.descriptorCount += b.descriptorCount * ci.max_set_per_pool;
                    found = true;
                    break;
                }
            }
            if (!found) {
                ci.sizes.emplace_back(b.descriptorType, b.descriptorCount * ci.max_set_per_pool);
            }
        }
    }
    MLE_ASSERT_LOG(!ci.sizes.empty(), "Descriptor pool sizes cannot be empty.");

    pool_ci_.setMaxSets(ci.max_set_per_pool);
    pool_ci_.setPoolSizes(ci.sizes);
    pool_ci_.flags = {};

    if (ci.free_individual_sets) {
        pool_ci_.flags |= vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    }
    if (ci.update_after_bind) {
        pool_ci_.flags |= vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
    }

    MLE_D("Creating GrowDescriptorPool, max sets per pool: {}, free individual sets: {}, update after bind: {}, sizes: [{}]", ci.max_set_per_pool,
          ci.free_individual_sets, ci.update_after_bind, fmt::join(ci.sizes, ", "));

    addPool();
}

std::vector<vk::DescriptorSet> GrowDescriptorPool::allocateMany(vk::DescriptorSetLayout layout, usize count) {
    std::vector<vk::DescriptorSet> ret;
    ret.reserve(count);

    for (usize i = 0; i < count; ++i) {
        ret.push_back(allocate(layout));
    }

    return ret;
};

GrowDescriptorPool::Pool& GrowDescriptorPool::addPool() {
    MLE_T("Adding new descriptor pool");
    auto new_pool = unwrap(Renderer::i().vkDevice().createDescriptorPool(pool_ci_));
    return pools_.emplace_back(Pool{.pool = new_pool, .allocated_sets = 0});
}

GrowDescriptorPool::~GrowDescriptorPool() {
    MLE_D("Destroying GrowDescriptorPool");
    for (auto& p : pools_) {
        Renderer::i().vkDevice().destroy(p.pool);
    }
};
}  // namespace mle
