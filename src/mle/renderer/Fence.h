/**
 * @file
 * @brief Vulkan fence wrapper
 */
#pragma once

#include "Types.h"
#include "mle/common/Result.h"

namespace mle::renderer {
/**
 * @brief A wrapper around a Vulkan fence extracted from the renderer pool.
 */
class Fence final {
  public:
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;
    Fence& operator=(Fence&&) = delete;

    /// Move constructor
    Fence(Fence&& other);

    /// Creates a new Fence instance by acquiring a Vulkan fence from the renderer.
    static Fence create();

    /// Destroys the fence and releases any associated Vulkan resources.
    ~Fence();

    /**
     * @brief Waits for the fence to become signaled.
     * @param timeout_ns The timeout in nanoseconds to wait for the fence. Default is 1 second.
     */
    void wait(u64 timeout_ns = 1'000'000'000) const;

    /**
     * @brief Asynchronously waits for the fence to become signaled.
     * @param callback The function to call when the fence is signaled.
     */
    void waitAsync(std::function<void(void)>&& callback = {});

    // void waitAsync(std::function<void(void)>&& callback, u64 timeout_ns = 1'000'000'000) const;

    auto get() { return o_; }    ///< Returns the underlying Vulkan fence handle.
    auto vkHnd() { return o_; }  ///< Returns the Vulkan fence handle (alias for `get()`).

  private:
    Fence() = default;  ///< Constructor is private. Use `create()` instead.
    void init();        ///< Initializes a new Fence instance by acquiring a Vulkan fence from the renderer.

  private:
    vk::Fence o_;  ///< Vulkan fence handle.
};
}  // namespace mle::renderer
