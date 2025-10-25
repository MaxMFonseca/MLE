#pragma once

#include <functional>

#include "Types.h"
#include "mle/math/Types2D.h"
#include "mle/renderer/CommandManager.h"
#include "mle/renderer/Image.h"
#include "mle/utils/Utils.h"

namespace mle {
// Please be mindfull that the lifetime of RenderingThread must be managed by the main thread.
// So create and end MUST be called from the main thread.
class RenderingThread {
  public:
    RenderingThread() = default;
    ~RenderingThread() = default;

    MLE_NO_COPY(RenderingThread);

    RenderingThread(RenderingThread&&) = default;
    RenderingThread& operator=(RenderingThread&&) = default;

    void init();
    void executeCommands();

    const auto& cmd() { return cmd_; }

    vk::CommandBuffer operator()() { return cmd_.get(); }

    auto getAttachmentDescription(u8 idx) { return color_attachments_.at(idx); }

    void setColorAttachment(const AttachmentInfo& attachment, u8 idx);
    void setColorAttachments(std::span<const AttachmentInfo> attachments);
    void removeAllColorAttachments();
    void setNoClear();
    void setDepthAttachment(const AttachmentInfo& attachment);

    void setPipeline(const Pipeline* p);
    void setPipeline(const std::string& name);

    void setViewport(Rectf viewport = {});
    void setScissor(Recti scissor = {});
    void setViewportAndScissor(Rectf viewport = {});
    void setLineWidth(f32 v) const;

    void bindVertexBuffer(BufferRef buffer, usize offset = 0) const;
    void bindInstanceBuffer(BufferRef buffer, usize offset = 0) const;
    void bindIndexBuffer(BufferRef buffer, usize offset = 0) const;
    void bindDescriptorSet(vk::DescriptorSet set, u32 binding) const;

    void pushConstants(const void* push_constants) const;
    void pushDescriptor(u32 set, std::span<const vk::WriteDescriptorSet> writes) const;

    void beginRendering(Recti render_area = {});
    void endRendering();

    void draw(u32 vertex_count, u32 instance_count, u32 first_vertex = 0, u32 first_instance = 0);
    void drawIndexed(u32 index_count, u32 instance_count, u32 first_index = 0, int vertex_offset = 0, u32 first_instance = 0);
    void dispatchCompute(int group_count_x, int group_count_y = 1, int group_count_z = 1) const;

    [[nodiscard]] Recti getRenderArea() const { return render_area_; }
    [[nodiscard]] auto getViewport() const { return viewport_; }
    [[nodiscard]] auto getScissor() const { return scissor_; }

    [[nodiscard]] ImageRef getColor0() const { return color_attachments_.at(0).image; }
    [[nodiscard]] vec2u getColor0Size() const { return getColor0()->getExtent(); }

  private:
    CommandBuffer cmd_{};

    bool in_rendering_ = false;

    std::array<AttachmentInfo, 4> color_attachments_{};
    AttachmentInfo depth_attachment_{};

    Rectf viewport_{0, 0, 0, 0};
    Recti scissor_{0, 0, 0, 0};

    const Pipeline* pipeline_{};
    Recti render_area_{};
};
}  // namespace mle
