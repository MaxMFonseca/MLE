#include "RenderingThread.h"

#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "Pipeline.h"
#include "Renderer.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"
#include "mle/renderer/detail/FrameRenderer.h"
#include "mle/ui/UI.h"
namespace mle::renderer {
void RenderingThread::init() {
    MLE_T("Initializing secondary rendering thread...");
    cmd_ = detail::getFrameRenderer().getCmdSecondary();

    vk::CommandBufferInheritanceInfo inheritance_info{};

    vk::CommandBufferBeginInfo cmd_begin_info{};
    cmd_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmd_begin_info.pInheritanceInfo = &inheritance_info;

    check(cmd_.begin(cmd_begin_info));
}

void RenderingThread::submit() {
    endRendering();
    check(cmd_.end());
    detail::getFrameRenderer().submitJob(cmd_);
}

void RenderingThread::setColorAttachments(std::vector<AttachmentInfo>&& attachments) {
    endRendering();
    color_attachments_ = std::move(attachments);
}

void RenderingThread::setDepthAttachment(const AttachmentInfo& attachment) {
    endRendering();
    depth_attachment_ = attachment;
}

void RenderingThread::setPipeline(PipelineRef p) {
    MLE_ASSERT(in_rendering_);
    if (p == pipeline_) {
        return;
    }
    pipeline_ = p;
    cmd_.bindPipeline(vk::PipelineBindPoint::eGraphics, p->get());
}

void RenderingThread::pushDescriptor(u32 set, const std::vector<vk::WriteDescriptorSet>& writes) {
    MLE_ASSERT(in_rendering_);
    MLE_ASSERT(pipeline_);
    cmd_.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, pipeline_->getPipelineLayout(), set, writes);
}

void RenderingThread::beginRendering(Recti render_area, bool can_clear) {
    if (render_area.size.x == 0) {
        render_area.size.x = color_attachments_.at(0).image->getExtent().x - render_area.pos.x;
    }
    if (render_area.size.y == 0) {
        render_area.size.y = color_attachments_.at(0).image->getExtent().y - render_area.pos.y;
    }

    if (in_rendering_) {
        if (render_area_ == render_area) {
            return;
        }
        endRendering();
    }

    render_area_ = render_area;

    vk::RenderingInfo render_info;
    render_info.setRenderArea({
        {render_area.pos.x, render_area.pos.y},
        {static_cast<u32>(render_area.size.x), static_cast<u32>(render_area.size.y)},
    });
    render_info.setViewMask(0);
    render_info.setLayerCount(1);

    std::vector<vk::RenderingAttachmentInfo> attachment_descriptions{color_attachments_.size()};
    for (usize i = 0; i < color_attachments_.size(); i++) {
        const auto& attachment = color_attachments_.at(i);
        auto& rattachment = attachment_descriptions.at(i);

        attachment.image->transitionState(cmd_, Image::State::COLOR_ATT);

        if (attachment.view) {
            rattachment.imageView = attachment.view;
        } else {
            rattachment.imageView = attachment.image->getView();
        }
        rattachment.clearValue = attachment.clear_value;
        rattachment.storeOp = attachment.store;
        rattachment.imageLayout = vk::ImageLayout::eAttachmentOptimal;

        rattachment.loadOp = attachment.load;
        if (rattachment.loadOp == vk::AttachmentLoadOp::eClear && !can_clear) {
            rattachment.loadOp = vk::AttachmentLoadOp::eLoad;
        }
    }
    render_info.setColorAttachments(attachment_descriptions);

    vk::RenderingAttachmentInfo depth_attachment;
    if (depth_attachment_.image || depth_attachment_.view) {
        depth_attachment_.image->transitionState(cmd_, Image::State::DEPTH_ATT);

        depth_attachment.imageView = !depth_attachment_.view ? depth_attachment_.image->getView() : depth_attachment_.view;
        depth_attachment.clearValue = depth_attachment_.clear_value;
        depth_attachment.storeOp = depth_attachment_.store;
        depth_attachment.imageLayout = vk::ImageLayout::eAttachmentOptimal;

        depth_attachment.loadOp = depth_attachment_.load;
        if (depth_attachment.loadOp == vk::AttachmentLoadOp::eClear && !can_clear) {
            depth_attachment.loadOp = vk::AttachmentLoadOp::eLoad;
        }

        render_info.setPDepthAttachment(&depth_attachment);
    }

    for (auto c : color_attachments_) {
        c.image->transitionState(cmd_, Image::State::COLOR_ATT);
    }

    cmd_.beginRendering(render_info);
    in_rendering_ = true;
}

void RenderingThread::endRendering() {
    if (in_rendering_) {
        cmd_.endRendering();
        in_rendering_ = false;
    }
}

void RenderingThread::setViewport(const Rectf& viewport) {
    MLE_ASSERT(in_rendering_);
    MLE_T("Setting viewport: pos = {}, size = {}", viewport.pos, viewport.size);

    vk::Viewport vv{};
    vv.x = viewport.pos.x;
    vv.y = viewport.pos.y;
    vv.minDepth = 0.0F;
    vv.maxDepth = 1.0F;

    if (viewport.size.x == 0.0F) {
        vv.width = static_cast<f32>(render_area_.size.x) - viewport.pos.x;
    } else {
        vv.width = viewport.size.x;
    }

    if (viewport.size.y == 0.0F) {
        vv.height = static_cast<f32>(render_area_.size.y) - viewport.pos.y;
    } else {
        vv.height = viewport.size.y;
    }

    vk::Rect2D scissor{};
    scissor.offset.x = static_cast<int>(viewport.pos.x);
    scissor.offset.y = static_cast<int>(viewport.pos.y);
    scissor.extent.width = static_cast<u32>(vv.width);
    scissor.extent.height = static_cast<u32>(vv.height);
    if (scissor.offset.x < 0) {
        scissor.extent.width += scissor.offset.x;
        scissor.offset.x = 0;
    }
    if (scissor.offset.y < 0) {
        scissor.extent.height += scissor.offset.y;
        scissor.offset.y = 0;
    }

    cmd_.setViewport(0, {vv});
    viewport_ = Rectf{vv.x, vv.y, vv.width, vv.height};

    cmd_.setScissor(0, scissor);
}

void RenderingThread::bindVertexBuffer(BufferRef buffer, usize offset) const {
    MLE_ASSERT(in_rendering_);
    cmd_.bindVertexBuffers(0, buffer->get(), offset);
}

void RenderingThread::bindInstanceBuffer(BufferRef buffer, usize offset) const {
    MLE_ASSERT(in_rendering_);
    MLE_ASSERT(pipeline_);
    MLE_ASSERT(pipeline_->hasInstance());
    cmd_.bindVertexBuffers(pipeline_->getFirstInstanceBinding(), buffer->get(), offset);
}

void RenderingThread::bindIndexBuffer(BufferRef buffer, usize offset) const {
    MLE_ASSERT(in_rendering_);
    cmd_.bindIndexBuffer(buffer->get(), offset, vk::IndexType::eUint32);
}

void RenderingThread::pushConstants(const void* push_constants) {
    MLE_ASSERT(in_rendering_);
    MLE_ASSERT(pipeline_);
    MLE_ASSERT(pipeline_->hasPushConstants());
    auto pc_size = pipeline_->getPushConstantSize();
    auto pc_frag_offset = pipeline_->getPushConstantFragOffset();
    auto pipeline_layout = pipeline_->getPipelineLayout();

    if (pc_frag_offset == max<u8>()) {
        cmd_.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, pc_size, push_constants);
    } else if (pc_frag_offset == 0) {
        cmd_.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, pc_size, push_constants);
    } else {
        cmd_.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, pc_frag_offset, push_constants);
        cmd_.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eFragment, pc_frag_offset, pc_size - pc_frag_offset,
                           rAs<const u8*>(push_constants) + pc_frag_offset);  // NOLINT
    }
}

void RenderingThread::bindDescriptorSet(vk::DescriptorSet set, u32 binding) const {
    MLE_ASSERT(in_rendering_);
    MLE_ASSERT(pipeline_);
    cmd_.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_->getPipelineLayout(), binding, {set}, {});
}

void RenderingThread::draw(int instance_count, int vertex_count) const {
    MLE_ASSERT(in_rendering_);
    MLE_ASSERT(pipeline_);
    cmd_.draw(vertex_count, instance_count, 0, 0);
}

void RenderingThread::drawIndexed(int instance_count, int index_count, usize index_offset) const {
    MLE_ASSERT(in_rendering_);
    MLE_ASSERT(pipeline_);
    cmd_.drawIndexed(index_count, instance_count, index_offset, 0, 0);
}
}  // namespace mle::renderer
