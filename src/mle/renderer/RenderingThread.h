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
    void init(bool primary = false);

    /// Ends the current command buffer recording
    void end();

    /// Returns the internal Vulkan command buffer used for issuing commands.
    auto cmd() { return cmd_; }

    /// Returns the currently active Vulkan rendering area.
    [[nodiscard]] auto getCurrentRenderArea() const { return rendering_data_.vk_rendering.renderArea; }

    void beginRendering(const RenderingInfo& info);
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
    bool primary_ = false;   ///< Indicates if this is the primary rendering thread.

    struct {
        PipelineRef pipeline{nullptr};           ///< Currently bound pipeline.
        PipelineRef previous_pipeline{nullptr};  ///< Previously bound pipeline, for state tracking.
        vk::RenderingInfo vk_rendering{};        ///< Vulkan rendering state.
    } rendering_data_;                           ///< State
};
}  // namespace mle::renderer
