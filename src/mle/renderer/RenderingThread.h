/**
 * @file
 * @brief Provides the RenderingThread class used for issuing Vulkan rendering commands in a thread context.
 */

#pragma once

#include "Types.h"

namespace mle::renderer {
/**
 * @brief Manages Vulkan command buffer recording for rendering in a thread-local context.
 *
 * The RenderingThread class encapsulates state related to Vulkan rendering operations such as
 * beginning a render pass, binding buffers, setting viewports, and issuing draw calls.
 * It maintains minimal internal state to track the currently bound pipeline and rendering region.
 */
class RenderingThread {
  public:
    RenderingThread(const RenderingThread&) = delete;
    RenderingThread(RenderingThread&&) = delete;
    RenderingThread& operator=(const RenderingThread&) = delete;
    RenderingThread& operator=(RenderingThread&&) = delete;

    /// Constructs an empty RenderingThread. Use `init()` before before anything.
    RenderingThread() = default;
    ~RenderingThread() = default;

    /**
     * @brief Initializes the rendering thread state.
     * @param primary Whether this is the primary rendering thread.
     */
    void init();

    /// Ends the current command buffer recording
    void submit();

    /// Returns the internal Vulkan command buffer used for issuing commands.
    auto cmd() { return cmd_; }

    void setColorAttachments(std::vector<AttachmentInfo>&& attachments);
    void setDepthAttachment(const AttachmentInfo& attachment);
    void setPipeline(PipelineRef p);
    void beginRendering(Recti render_area);
    void endRendering();
    void setViewport(const Rectf& viewport = {}) const;
    void bindVertexBuffer(BufferRef buffer, usize offset = 0) const;
    void bindInstanceBuffer(BufferRef buffer, usize offset = 0) const;
    void bindIndexBuffer(BufferRef buffer, usize offset = 0) const;
    void pushConstants(const void* push_constants);
    void draw(int instance_count, int index_count) const;

  private:
    friend class Pipeline;
    void beginRendering(const Pipeline* pipeline, vk::RenderingInfo info);

  private:
    vk::CommandBuffer cmd_;  ///< Thread-local Vulkan command buffer for recording rendering commands.

    bool in_rendering_ = false;  ///< Flag indicating if currently in a rendering pass.

    std::vector<AttachmentInfo> color_attachments_{};  ///< Framebuffer attachments.
    AttachmentInfo depth_attachment_{};                ///< Depth attachment info.

    PipelineRef pipeline_{nullptr};  ///< Currently bound pipeline.
    Recti render_area_{};            ///< Current rendering area.
};
}  // namespace mle::renderer
