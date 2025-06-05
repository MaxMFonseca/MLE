#include "Renderer.h"

#include <vulkan/vk_enum_string_helper.h>

#include <ranges>

#include "Buffer.h"
#include "detail/FencePool.h"
#include "detail/VkContext.h"
#include "mle/common/Assert.h"
#include "mle/core/Core.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/detail/CommandPoolManager.h"
#include "mle/renderer/detail/FrameRenderer.h"
#include "mle/window/Window.h"

namespace mle::renderer {
namespace {
class Impl {
  public:
    // TODO: State

  public:
    inline void init();
    void shutdown();
    void addOnShutdown(std::function<void(void)>&& func);

    Result beginFrame();            ///< Begins a new rendering frame.
    void endFrame(ImageRef image);  ///< Ends the current rendering frame and presents the given image.

    auto& getED() { return ed_; }                 ///< Returns the event dispatcher instance for the renderer.
    auto& getVk() { return vk_; }                 ///< Returns the Vulkan context instance.
    auto getDevice() { return vk_.getDevice(); }  ///< Returns the Vulkan device handle.
    auto getVma() { return vk_.getVma(); }        ///< Returns the Vulkan Memory Allocator instance.
    auto& getFencePool() { return fence_pool_; }  ///< Returns the fence pool for managing Vulkan fences.

  private:
  private:
    ED ed_;

    detail::VkContext vk_;
    detail::CommandPoolManager command_pool_manager_;
    detail::FencePool fence_pool_;
    detail::FrameRenderer frame_renderer_;

    std::vector<std::function<void(void)>> shutdown_delete_stack_;
};
std::unique_ptr<Impl> i_;  // NOLINT

void Impl::init() {
    MLE_I("Initializing the Renderer");

    vk_.init();

    command_pool_manager_.init();
    shutdown_delete_stack_.emplace_back([this]() { command_pool_manager_.reset(); });

    frame_renderer_.init();
    shutdown_delete_stack_.emplace_back([this]() { frame_renderer_.reset(); });

    MLE_I("Renderer initialized successfully!");
}

void Impl::shutdown() {
    MLE_I("Cleaning up Renderer");

    vk_.waitIdle();

    for (auto& it : std::ranges::reverse_view(shutdown_delete_stack_)) {
        it();
    }

    MLE_D("Live instances after shutdown:");
    LiveCounter<Buffer>::listActiveInstances("Buffer");
    LiveCounter<Image>::listActiveInstances("Image");
}

void Impl::addOnShutdown(std::function<void(void)>&& func) {
    shutdown_delete_stack_.emplace_back(std::move(func));
}

Result Impl::beginFrame() {
    return frame_renderer_.beginFrame();
}

void Impl::endFrame(ImageRef image) {
    frame_renderer_.endFrame(image);
}
}  // namespace

void init([[maybe_unused]] const CI& ci) {
    MLE_ASSERT(!i_);
    i_ = std::make_unique<Impl>();
    i_->init();
}

void shutdown() {
    if (i_) {
        MLE_I("Shutting down Renderer");
        i_->shutdown();
        i_.reset();
    }
}

void addOnShutdown(std::function<void(void)>&& func) {
    MLE_ASSERT(i_);
    i_->addOnShutdown(std::move(func));
}

Result beginFrame() {
    MLE_ASSERT(i_);
    return i_->beginFrame();
}

void endFrame(ImageRef image) {
    MLE_ASSERT(i_);
    i_->endFrame(image);
}

namespace detail {
ED& getED() {
    MLE_ASSERT(i_);
    return i_->getED();
}

VkContext& getVk() {
    MLE_ASSERT(i_);
    return i_->getVk();
}

vk::Device getDevice() {
    MLE_ASSERT(i_);
    return i_->getDevice();
}

VmaAllocator getVma() {
    MLE_ASSERT(i_);
    return i_->getVma();
}

FencePool& getFencePool() {
    MLE_ASSERT(i_);
    return i_->getFencePool();
}

void waitIdle() {
    MLE_ASSERT(i_);
    i_->getVk().waitIdle();
}
}  // namespace detail
}  // namespace mle::renderer
