#include "Renderer.h"

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
#include "mle/renderer/detail/ModelCache.h"
#include "mle/renderer/detail/PipelineCache.h"
#include "mle/renderer/detail/ShaderCache.h"
#include "mle/renderer/detail/TextureCache.h"
#include "mle/window/Window.h"
#include "sol/stack_core.hpp"

namespace mle::renderer {
namespace {
class Impl {
  public:
    inline void init();
    void createSamplers();  ///< Creates default samplers for the renderer.
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
    auto& getModelCache() { return model_cache_; }                   ///< Returns the model cache for managing 3D models.
    auto& getPipelineCache() { return pipeline_cache_; }
    auto getNearesSample() { return nearest_sampler_; }
    auto getLinearSampler() { return linear_sampler_; }
    auto getShadowSampler() { return shadow_sampler_; }

    CommandPool& getOTSPool(CmdType type);
    vk::CommandBuffer getOTSCmd(CmdType type);
    void submitOTSWait(CmdType cmd_type, vk::CommandBuffer cmd);
    void submitOTSAsync(CmdType cmd_type, vk::CommandBuffer cmd, std::move_only_function<void(void)>&& callback);
    void submitOTSAsync(CmdType cmd_type, vk::SubmitInfo2 submit_info, std::move_only_function<void(void)>&& callback = nullptr);

  private:
    ED ed_;

    CommandPool g_pool_, t_pool_;

    detail::VkContext vk_;
    detail::CommandPoolManager command_pool_manager_;
    detail::FencePool fence_pool_;
    detail::FrameRenderer frame_renderer_;
    detail::ShaderCache shader_cache_;
    detail::TextureCache texture_cache_;
    detail::ModelCache model_cache_;
    detail::PipelineCache pipeline_cache_;

    std::vector<std::function<void(void)>> shutdown_delete_stack_;

    vk::Sampler linear_sampler_{};
    vk::Sampler nearest_sampler_{};
    vk::Sampler shadow_sampler_{};
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

    model_cache_.init();
    shutdown_delete_stack_.emplace_back([this]() { model_cache_.reset(); });

    pipeline_cache_.init();
    shutdown_delete_stack_.emplace_back([this]() { pipeline_cache_.shutdown(); });

    shutdown_delete_stack_.emplace_back([this]() { fence_pool_.reset(); });

    createSamplers();

    MLE_I("Renderer initialized successfully!");
}

void Impl::createSamplers() {
    MLE_I("Creating default samplers");

    vk::SamplerCreateInfo linear_sampler_ci{};
    linear_sampler_ci.magFilter = vk::Filter::eLinear;
    linear_sampler_ci.minFilter = vk::Filter::eLinear;
    linear_sampler_ci.mipmapMode = vk::SamplerMipmapMode::eLinear;
    linear_sampler_ci.addressModeU = vk::SamplerAddressMode::eRepeat;
    linear_sampler_ci.addressModeV = vk::SamplerAddressMode::eRepeat;
    linear_sampler_ci.addressModeW = vk::SamplerAddressMode::eRepeat;
    linear_sampler_ci.anisotropyEnable = vk::False;
    linear_sampler_ci.borderColor = vk::BorderColor::eFloatOpaqueWhite;
    linear_sampler_ci.compareEnable = vk::False;
    linear_sampler_ci.maxLod = VK_LOD_CLAMP_NONE;
    linear_sampler_ci.maxAnisotropy = 1.0F;

    MLE_T("Creating linear sampler");
    linear_sampler_ = unwrap(getDevice().createSampler(linear_sampler_ci));

    vk::SamplerCreateInfo nearest_sampler_ci{};
    nearest_sampler_ci.magFilter = vk::Filter::eNearest;
    nearest_sampler_ci.minFilter = vk::Filter::eNearest;
    nearest_sampler_ci.mipmapMode = vk::SamplerMipmapMode::eNearest;
    nearest_sampler_ci.addressModeU = vk::SamplerAddressMode::eRepeat;
    nearest_sampler_ci.addressModeV = vk::SamplerAddressMode::eRepeat;
    nearest_sampler_ci.addressModeW = vk::SamplerAddressMode::eRepeat;
    nearest_sampler_ci.anisotropyEnable = vk::False;
    nearest_sampler_ci.borderColor = vk::BorderColor::eFloatOpaqueWhite;
    nearest_sampler_ci.compareEnable = vk::False;
    nearest_sampler_ci.maxLod = VK_LOD_CLAMP_NONE;
    nearest_sampler_ci.maxAnisotropy = 1.0F;

    MLE_T("Creating nearest sampler");
    nearest_sampler_ = unwrap(getDevice().createSampler(nearest_sampler_ci));

    vk::SamplerCreateInfo shadow_sampler_ci{};
    shadow_sampler_ci.magFilter = vk::Filter::eLinear;
    shadow_sampler_ci.minFilter = vk::Filter::eLinear;
    shadow_sampler_ci.mipmapMode = vk::SamplerMipmapMode::eLinear;
    shadow_sampler_ci.addressModeU = vk::SamplerAddressMode::eClampToBorder;
    shadow_sampler_ci.addressModeV = vk::SamplerAddressMode::eClampToBorder;
    shadow_sampler_ci.addressModeW = vk::SamplerAddressMode::eClampToBorder;
    shadow_sampler_ci.anisotropyEnable = vk::False;
    shadow_sampler_ci.borderColor = vk::BorderColor::eFloatOpaqueBlack;
    shadow_sampler_ci.compareEnable = vk::True;
    shadow_sampler_ci.compareOp = vk::CompareOp::eLessOrEqual;
    shadow_sampler_ci.maxLod = VK_LOD_CLAMP_NONE;
    shadow_sampler_ci.maxAnisotropy = 1.0F;

    MLE_T("Creating shadow sampler");
    shadow_sampler_ = unwrap(getDevice().createSampler(shadow_sampler_ci));

    addOnShutdown([this]() {
        getDevice().destroy(linear_sampler_);
        getDevice().destroy(nearest_sampler_);
        getDevice().destroy(shadow_sampler_);
    });
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
}

Result Impl::beginFrame() {
    auto fr_begin_result = frame_renderer_.beginFrame();
    if (fr_begin_result != Result::OK) {
        return fr_begin_result;
    }
    texture_cache_.frameBegin();
    return Result::OK;
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

void Impl::submitOTSAsync(CmdType cmd_type, vk::CommandBuffer cmd, std::move_only_function<void(void)>&& callback) {
    getOTSPool(cmd_type).submitAsync(cmd, std::move(callback));
}

void Impl::submitOTSAsync(CmdType cmd_type, vk::SubmitInfo2 submit_info, std::move_only_function<void(void)>&& callback) {
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

void deleteAfterFrame(ImageHnd&& image) {
    MLE_ASSERT(i_);
    i_->getFrameRenderer().deleteAfterFrame(std::move(image));
}

void deleteAfterFrame(BufferHnd&& buffer) {
    MLE_ASSERT(i_);
    i_->getFrameRenderer().deleteAfterFrame(std::move(buffer));
}

vk::Format getDefaultColorFormat() {
    MLE_ASSERT(i_);
    return i_->getVk().getColorFormat();
}

vk::Format getDepthFormat() {
    MLE_ASSERT(i_);
    return i_->getVk().getDepthFormat();
}

ShaderRef getShader(const std::string& name) {
    MLE_ASSERT(i_);
    return i_->getShaderCache().get(name);
}

Texture addTexture(const std::string& name, ImageHnd&& img) {
    return i_->getTextureCache().add(name, std::move(img));
}

Texture getTexture(const std::string& name) {
    MLE_ASSERT(i_);
    return i_->getTextureCache().get(name);
}

ModelRef getModel(const std::string& name) {
    MLE_ASSERT(i_);
    return i_->getModelCache().get(name);
}

ModelRef loadModel(const std::string& name) {
    MLE_ASSERT(i_);
    return i_->getModelCache().loadModel(name);
}

ModelRef addModel(const std::string& name, const UploadModelData& upload_data) {
    MLE_ASSERT(i_);
    return i_->getModelCache().add(name, upload_data);
}

vk::CommandBuffer getOTSCmd(CmdType type) {
    MLE_ASSERT(i_);
    return i_->getOTSCmd(type);
}

void submitOTSWait(CmdType cmd_type, vk::CommandBuffer cmd) {
    MLE_ASSERT(i_);
    i_->submitOTSWait(cmd_type, cmd);
}

void submitOTSAsync(CmdType cmd_type, vk::CommandBuffer cmd, std::move_only_function<void(void)>&& callback) {
    MLE_ASSERT(i_);
    i_->submitOTSAsync(cmd_type, cmd, std::move(callback));
}

void submitOTSAsync(CmdType cmd_type, vk::SubmitInfo2 submit_info, std::move_only_function<void(void)>&& callback) {
    MLE_ASSERT(i_);
    i_->submitOTSAsync(cmd_type, submit_info, std::move(callback));
}

vk::DescriptorSetLayout getTexturesDescriptorSetLayout() {
    MLE_ASSERT(i_);
    return i_->getTextureCache().getDescriptorSetLayout();
}

void bindTexturesDSet(RenderingThread& thread) {
    MLE_ASSERT(i_);
    i_->getTextureCache().bindTexturesDSet(thread);
}

void enqueueTextureUpdateJob(TextureUpdateJobData&& data) {
    MLE_ASSERT(i_);
    i_->getTextureCache().enqueueTextureUpdateJob(std::move(data));
}

vk::CommandBuffer getFrameSecondaryCmd() {
    MLE_ASSERT(i_);
    return i_->getFrameRenderer().getCmdSecondary();
}

void submitJobOnFrame(vk::CommandBuffer cmd) {
    MLE_ASSERT(i_);
    i_->getFrameRenderer().submitJob(cmd);
}

vk::Sampler getLinearSampler() {
    MLE_ASSERT(i_);
    return i_->getLinearSampler();
}

vk::Sampler getNearestSampler() {
    MLE_ASSERT(i_);
    return i_->getNearesSample();
}

vk::Sampler getShadowSampler() {
    MLE_ASSERT(i_);
    return i_->getShadowSampler();
}

BufferSlice getHostVisibleBuffer(usize size, vk::BufferUsageFlags usage) {
    MLE_ASSERT(i_);
    return i_->getFrameRenderer().getHostVisibleBuffer(size, usage);
}

PipelineRef setPipeline(const std::string& name, const Pipeline::CI& pipeline_ci) {
    MLE_ASSERT(i_);
    return i_->getPipelineCache().setPipeline(name, pipeline_ci);
}

PipelineRef getPipeline(const std::string& name) {
    MLE_ASSERT(i_);
    return i_->getPipelineCache().getPipeline(name);
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

vk::CommandBuffer getFramePrimaryCmd() {
    MLE_ASSERT(i_);
    return i_->getFrameRenderer().getCmd();
}
}  // namespace detail
}  // namespace mle::renderer
