#pragma once

#include "Events.h"
#include "ShaderModule.h"
#include "Types.h"
#include "Utils.h"
#include "mle/common/Color.h"
#include "mle/common/Result.h"
#include "mle/renderer/Buffer.h"
#include "mle/renderer/VkContext.h"

namespace mle::renderer {
struct CreateInfo {};
using CI = CreateInfo;

void init(CI&& ci);
void shutdown();

Result beginFrame();
void endFrame(ImageRef frame_image);

void beginRendering(RenderingInfo render_info);
void endRendering();
void bindGraphicsPipeline(PipelineRef pipeline);

vk::CommandBuffer beginTransferOTS();
void endTransferOTS();

void waitIdle();

void addToFrameDeleteQueue(std::function<void(void)>&& func);
void addToFrameDeleteQueue(BufferHnd&& hnd);
void addToFrameDeleteQueue(ImageHnd&& hnd);

void addToShutdownDeleteQueue(std::function<void(void)>&& func);

BufferRef createBufferForFrame(const Buffer::CI& ci);

ED& getED();
VkContext& getVk();
usize getFrameNumber();
usize getFrameId();
vk::CommandBuffer getFrameMainCommandBuffer();
Recti getCurrentRenderArea();
ImageRef getDefaultTextureImage();
vk::Format getDefaultColorFormat();
vk::Format getDepthFormat();
ShaderModuleRef getShaderModule(const fs::path& path);

PipelineGetResult getPipeline(const std::string& name);

bool isFrameActive();
}  // namespace mle::renderer
