#include "Renderer.h"

#include <vulkan/vk_enum_string_helper.h>

#include <ranges>
#include <vulkan/vulkan_handles.hpp>

#include "Buffer.h"
#include "detail/FencePool.h"
#include "detail/VkContext.h"
#include "mle/common/Assert.h"
#include "mle/core/Core.h"
#include "mle/renderer/CommandPool.h"
#include "mle/renderer/Shader.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"
#include "mle/renderer/detail/CommandPoolManager.h"
#include "mle/renderer/detail/FrameRenderer.h"
#include "mle/renderer/detail/ShaderCache.h"
#include "mle/renderer/detail/TextureCache.h"
#include "mle/window/Window.h"

namespace mle::renderer {
namespace {
class Impl {
  public:
    inline void init();
    inline void shutdown();
    void addOnShutdown(std::function<void(void)>&& func);

    inline void update();

    Result beginFrame();            ///< Begins a new rendering frame.
    void endFrame(ImageRef image);  ///< Ends the current rendering frame and presents the given image.

    auto& getED() { return ed_; }                                    ///< Returns the event dispatcher instance for the renderer.
    auto& getVk() { return vk_; }                                    ///< Returns the Vulkan context instance.
    auto getDevice() { return vk_.getDevice(); }                     ///< Returns the Vulkan device handle.
    auto getVma() { return vk_.getVma(); }                           ///< Returns the Vulkan Memory Allocator instance.
    auto& getFencePool() { return fence_pool_; }                     ///< Returns the fence pool for managing Vulkan fences.
    auto& getCommandPoolManager() { return command_pool_manager_; }  ///< Returns the command pool manager.
    auto& getFrameRenderer() { return frame_renderer_; }             ///< Returns the frame renderer for managing frame-specific operations.
    auto& getShaderCache() { return shader_cache_; }                 ///< Returns the shader cache for managing shaders.
    auto& getTextureCache() { return texture_cache_; }               ///< Returns the texture cache for managing textures.

    CommandPool& getOTSPool(CmdType type);
    vk::CommandBuffer getOTSCmd(CmdType type);
    void submitOTSWait(CmdType cmd_type, vk::CommandBuffer cmd);
    void submitOTSAsync(CmdType cmd_type, vk::CommandBuffer cmd, std::function<void(void)>&& callback);
    void submitOTSAsync(CmdType cmd_type, vk::SubmitInfo2 submit_info, std::function<void(void)>&& callback = nullptr);

  private:
  private:
    ED ed_;

    CommandPool g_pool_, t_pool_;

    detail::VkContext vk_;
    detail::CommandPoolManager command_pool_manager_;
    detail::FencePool fence_pool_;
    detail::FrameRenderer frame_renderer_;
    detail::ShaderCache shader_cache_;
    detail::TextureCache texture_cache_;

    std::vector<std::function<void(void)>> shutdown_delete_stack_;
};
std::unique_ptr<Impl> i_;  // NOLINT

void Impl::init() {
    MLE_I("Initializing the Renderer");

    vk_.init();

    command_pool_manager_.init();
    g_pool_.init(CmdType::GRAPHICS);
    t_pool_.init(CmdType::TRANSFER);

    shutdown_delete_stack_.emplace_back([this]() {
        g_pool_.reset();
        t_pool_.reset();
        command_pool_manager_.reset();
    });

    frame_renderer_.init();
    shutdown_delete_stack_.emplace_back([this]() { frame_renderer_.reset(); });

    shader_cache_.init();
    shutdown_delete_stack_.emplace_back([this]() { shader_cache_.reset(); });

    texture_cache_.init();
    shutdown_delete_stack_.emplace_back([this]() { texture_cache_.reset(); });

    shutdown_delete_stack_.emplace_back([this]() { fence_pool_.reset(); });

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
    LiveCounter<Pipeline>::listActiveInstances("Pipeline");
    LiveCounter<Shader>::listActiveInstances("Shader");
}

void Impl::addOnShutdown(std::function<void(void)>&& func) {
    shutdown_delete_stack_.emplace_back(std::move(func));
}

void Impl::update() {
    fence_pool_.update();
    texture_cache_.update();
}

Result Impl::beginFrame() {
    return frame_renderer_.beginFrame();
}

void Impl::endFrame(ImageRef image) {
    frame_renderer_.endFrame(image);
}

CommandPool& Impl::getOTSPool(CmdType type) {
    switch (type) {
        case CmdType::GRAPHICS:
            return g_pool_;
        case CmdType::TRANSFER:
            return t_pool_;
        default:
            MLE_UNREACHABLE_LOG("Invalid command pool type: {}", type);
            return g_pool_;  // This will never be reached, but avoids compiler warnings.
    }
}

vk::CommandBuffer Impl::getOTSCmd(CmdType type) {
    return getOTSPool(type).getCmd();
}

void Impl::submitOTSWait(CmdType cmd_type, vk::CommandBuffer cmd) {
    getOTSPool(cmd_type).submitWait(cmd);
}

void Impl::submitOTSAsync(CmdType cmd_type, vk::CommandBuffer cmd, std::function<void(void)>&& callback) {
    getOTSPool(cmd_type).submitAsync(cmd, std::move(callback));
}

void Impl::submitOTSAsync(CmdType cmd_type, vk::SubmitInfo2 submit_info, std::function<void(void)>&& callback) {
    getOTSPool(cmd_type).submitAsync(submit_info, std::move(callback));
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

void update() {
    MLE_ASSERT(i_);
    i_->update();
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

void addToCallOnFrameEnd(std::function<void()>&& fn) {
    MLE_ASSERT(i_);
    i_->getFrameRenderer().addToCallOnFrameEnd(std::move(fn));
}

vk::Format getDefaultColorFormat() {
    MLE_ASSERT(i_);
    return i_->getVk().getColorFormat();
}

ShaderRef getShader(const std::string& name, bool engine) {
    MLE_ASSERT(i_);
    return i_->getShaderCache().get(name, engine);
}

Texture getTexture(const std::string& name, bool engine) {
    MLE_ASSERT(i_);
    return i_->getTextureCache().get(name, engine);
}

vk::CommandBuffer getOTSCmd(CmdType type) {
    MLE_ASSERT(i_);
    return i_->getOTSCmd(type);
}

void submitOTSWait(CmdType cmd_type, vk::CommandBuffer cmd) {
    MLE_ASSERT(i_);
    i_->submitOTSWait(cmd_type, cmd);
}

void submitOTSAsync(CmdType cmd_type, vk::CommandBuffer cmd, std::function<void(void)>&& callback) {
    MLE_ASSERT(i_);
    i_->submitOTSAsync(cmd_type, cmd, std::move(callback));
}

void submitOTSAsync(CmdType cmd_type, vk::SubmitInfo2 submit_info, std::function<void(void)>&& callback) {
    MLE_ASSERT(i_);
    i_->submitOTSAsync(cmd_type, submit_info, std::move(callback));
}

u32 useTexture(RenderingThread& thread, u32 idx) {
    MLE_ASSERT(i_);
    return i_->getTextureCache().use(thread, idx);
}

vk::DescriptorSetLayout getTexturesDescriptorSetLayout() {
    MLE_ASSERT(i_);
    return i_->getTextureCache().getDescriptorSetLayout();
}

void bindTexturesDSet(RenderingThread& thread) {
    MLE_ASSERT(i_);
    i_->getTextureCache().bindTexturesDSet(thread);
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

CommandPoolManager& getCommandPoolManager() {
    MLE_ASSERT(i_);
    return i_->getCommandPoolManager();
}

FrameRenderer& getFrameRenderer() {
    MLE_ASSERT(i_);
    return i_->getFrameRenderer();
}
}  // namespace detail
}  // namespace mle::renderer
