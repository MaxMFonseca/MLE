#include "RenderingThread.h"

#include "mle/renderer/Renderer.h"

namespace mle {
RenderingThread::RenderingThread() :
    cmd_(Renderer::i().frameRenderer().getSecondaryCommandBuffer()) {
}

void RenderingThread::setColorAttachments(std::span<const AttachmentInfo> attachments) {
    removeAllColorAttachments();
    for (usize i = 0; i < attachments.size() && i < color_attachments_.size(); i++) {
        color_attachments_.at(i) = attachments[i];
    }
};

void RenderingThread::removeAllColorAttachments() {
    endRendering();
    for (auto& att : color_attachments_) {
        att = {};
    }
}

void RenderingThread::setNoClear() {
    endRendering();
    for (auto& att : color_attachments_) {
        att.load_op = vk::AttachmentLoadOp::eLoad;
    }
    depth_attachment_.load_op = vk::AttachmentLoadOp::eLoad;
};

void RenderingThread::endRendering() {
    if (in_rendering_) {
        cmd_().endRendering();
        in_rendering_ = false;
        pipeline_ = nullptr;
        viewport_ = {};
        scissor_ = {};
    }
}

void RenderingThread::setColorAttachment(const AttachmentInfo& attachment, u8 idx) {
    if (idx >= color_attachments_.size()) {
        MLE_E("Tried to set color attachment at invalid index {}", idx);
        return;
    }
    endRendering();
    color_attachments_.at(idx) = attachment;
}

void RenderingThread::setDepthAttachment(const AttachmentInfo& attachment) {
    endRendering();
    depth_attachment_ = attachment;
}

void RenderingThread::setPipeline(const Pipeline* p) {
    MLE_ASSERT_LOG(p->isCompute() || in_rendering_, "Pipeline can only be set outside rendering.");
    MLE_ASSERT_LOG(!(p->isCompute() && in_rendering_), "Compute pipeline can not be set inside rendering.");
    MLE_ASSERT_LOG(p != nullptr, "Cannot set a null pipeline.");

    if (p == pipeline_) {
        return;
    }
    pipeline_ = p;
    cmd_().bindPipeline(pipeline_->isCompute() ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics, p->get());
}

void RenderingThread::setPipeline(const std::string& name) {
    setPipeline(&Renderer::i().pipelineCache().getPipeline(name));
}

void RenderingThread::setViewport(Rectf viewport) {
    MLE_ASSERT_LOG(in_rendering_, "Viewport can only be set inside rendering.");

    if (viewport.width() == 0.0F) {
        viewport.setWidth(as<f32>(render_area_.width()) - viewport.left());
    }
    if (viewport.height() == 0.0F) {
        viewport.setHeight(as<f32>(render_area_.height()) - viewport.top());
    }

    vk::Viewport vv{};
    vv.x = viewport.left();
    vv.y = viewport.top();
    vv.width = viewport.width();
    vv.height = viewport.height();
    vv.minDepth = 0.0F;
    vv.maxDepth = 1.0F;

    cmd_().setViewport(0, {vv});
    viewport_ = viewport;
}

void RenderingThread::setScissor(Recti scissor) {
    MLE_ASSERT_LOG(in_rendering_, "Scissor can only be set inside rendering.");

    if (scissor.width() == 0) {
        scissor.setWidth(render_area_.width() - scissor.left());
    }
    if (scissor.height() == 0) {
        scissor.setHeight(render_area_.height() - scissor.top());
    }

    vk::Rect2D ss{};
    ss.offset.x = scissor.left();
    ss.offset.y = scissor.top();
    ss.extent = toVkExtent2D(scissor.size());
    if (ss.offset.x < 0) {
        ss.extent.width += ss.offset.x;
        ss.offset.x = 0;
    }
    if (ss.offset.y < 0) {
        ss.extent.height += ss.offset.y;
        ss.offset.y = 0;
    }

    cmd_().setScissor(0, ss);
    scissor_ = scissor;
}

void RenderingThread::setViewportAndScissor(Rectf viewport) {
    setViewport(viewport);
    setScissor(viewport_.asI32());
}

void RenderingThread::setLineWidth(f32 v) const {
    MLE_ASSERT_LOG(in_rendering_, "Line width can only be set inside rendering.");
    if (v <= 0.0F) {
        MLE_W("Line width must be greater than 0.0F, got: {}. Using 1.0F instead.", v);
        v = 1.0F;
    }
    cmd_().setLineWidth(v);
}

void RenderingThread::bindVertexBuffer(BufferRef buffer, usize offset) const {
    MLE_ASSERT_LOG(in_rendering_, "Vertex buffer can only be bound inside rendering.");
    MLE_ASSERT_LOG(buffer, "Cannot bind a null vertex buffer.");

    cmd_().bindVertexBuffers(0, buffer->get(), offset);
}

void RenderingThread::bindInstanceBuffer(BufferRef buffer, usize offset) const {
    MLE_ASSERT_LOG(buffer, "Cannot bind a null instance buffer.");
    MLE_ASSERT_LOG(pipeline_, "Pipeline must be set before binding instance buffer.");
    MLE_ASSERT_LOG(pipeline_->hasInstance(), "Pipeline does not support instancing.");
    MLE_ASSERT_LOG(in_rendering_, "Instance buffer can only be bound inside rendering.");

    int binding = pipeline_->getFirstInstanceBinding() == 0 ? 0 : 1;
    cmd_().bindVertexBuffers(binding, buffer->get(), offset);
}

void RenderingThread::bindIndexBuffer(BufferRef buffer, usize offset) const {
    MLE_ASSERT_LOG(buffer, "Cannot bind a null index buffer.");
    MLE_ASSERT_LOG(in_rendering_, "Index buffer can only be bound inside rendering.");

    cmd_().bindIndexBuffer(buffer->get(), offset, vk::IndexType::eUint32);
}

void RenderingThread::bindDescriptorSet(vk::DescriptorSet set, u32 binding) const {
    MLE_ASSERT_LOG(set, "Cannot bind a null descriptor set.");
    MLE_ASSERT_LOG(pipeline_, "Pipeline must be set before binding descriptor set.");
    MLE_ASSERT_LOG(pipeline_->isCompute() || in_rendering_, "Descriptor set must be bound inside rendering.");

    cmd_().bindDescriptorSets(pipeline_->isCompute() ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics, pipeline_->getPipelineLayout(),
                              binding, {set}, {});
}

void RenderingThread::pushConstants(const void* push_constants) const {
    MLE_ASSERT_LOG(pipeline_, "Pipeline must be set before pushing constants.");
    MLE_ASSERT_LOG(pipeline_->isCompute() || in_rendering_, "Push constants must be done inside rendering.");
    MLE_ASSERT_LOG(pipeline_->hasPushConstants(), "Pipeline does not have push constants.");

    auto pc_size = pipeline_->getPushConstantSize();
    auto pc_frag_offset = pipeline_->getPushConstantFragOffset();
    auto pipeline_layout = pipeline_->getPipelineLayout();

    if (pipeline_->isCompute()) {
        cmd_().pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, pc_size, push_constants);
        return;
    }

    if (pc_frag_offset == max<u8>()) {
        cmd_().pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, pc_size, push_constants);
    } else if (pc_frag_offset == 0) {
        cmd_().pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, pc_size, push_constants);
    } else {
        cmd_().pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, pc_frag_offset, push_constants);
        cmd_().pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eFragment, pc_frag_offset, pc_size - pc_frag_offset,
                             rAs<const u8*>(push_constants) + pc_frag_offset);  // NOLINT
    }
};

void RenderingThread::pushDescriptor(u32 set, std::span<const vk::WriteDescriptorSet> writes) const {
    MLE_ASSERT_LOG(pipeline_, "Pipeline must be set before pushing descriptor.");
    MLE_ASSERT_LOG(pipeline_->isCompute() || in_rendering_, "Push descriptor must be done inside rendering.");

    cmd_().pushDescriptorSetKHR(pipeline_->isCompute() ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics, pipeline_->getPipelineLayout(),
                                set, writes);
}

void RenderingThread::beginRendering(Recti render_area) {
    MLE_ASSERT_LOG(!in_rendering_, "Already in rendering.");
    MLE_ASSERT_LOG(color_attachments_.at(0).image || depth_attachment_.image, "No color or depth attachment set.");

    vec2i max_render_area{0};
    if (color_attachments_.at(0).image) {
        max_render_area = color_attachments_.at(0).image->getExtent();
    } else {
        max_render_area = depth_attachment_.image->getExtent();
    }

    if (render_area.width() == 0) {
        render_area.setWidth(max_render_area.x - render_area.left());
    }
    if (render_area.height() == 0) {
        render_area.setHeight(max_render_area.y - render_area.top());
    }

    MLE_ASSERT_LOG(render_area.left() >= 0 && render_area.top() >= 0, "Render area position must be non-negative.");
    MLE_ASSERT_LOG(render_area.right() <= max_render_area.x && render_area.bottom() <= max_render_area.y,
                   "Render area exceeds maximum extent: {}x{}, got right: {}, bottom: {}", max_render_area.x, max_render_area.y, render_area.right(),
                   render_area.bottom());

    render_area_ = render_area;

    vk::RenderingInfo rendering_info;
    rendering_info.setRenderArea(vk::Rect2D{
        {render_area.left(), render_area.top()},
        toVkExtent2D(render_area.size()),
    });
    rendering_info.setViewMask(0);
    rendering_info.setLayerCount(1);

    std::vector<vk::RenderingAttachmentInfo> attachment_descriptions{};
    for (const auto& attachment : color_attachments_) {
        if (!attachment.image) {
            break;
        }

        attachment.image->transitionState(cmd_, Image::State::COLOR_ATT);

        vk::RenderingAttachmentInfo color_attachment{};

        color_attachment.setImageView(attachment.view ? attachment.view : attachment.image->getDefaultView());
        color_attachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
        if (attachment.load_op == vk::AttachmentLoadOp::eClear) {
            color_attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
            color_attachment.setClearValue(attachment.clear_value);
        } else {
            color_attachment.setLoadOp(vk::AttachmentLoadOp::eLoad);
        }
        color_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);

        attachment_descriptions.push_back(color_attachment);
    }

    rendering_info.setColorAttachments(attachment_descriptions);

    vk::RenderingAttachmentInfo depth_attachment{};
    if (depth_attachment_.image) {
        depth_attachment_.image->transitionState(cmd_, Image::State::DEPTH_ATT);

        depth_attachment.setImageView(depth_attachment_.view ? depth_attachment_.view : depth_attachment_.image->getDefaultView());
        depth_attachment.setImageLayout(vk::ImageLayout::eAttachmentOptimal);
        depth_attachment.setClearValue(depth_attachment_.clear_value);

        if (depth_attachment_.load_op == vk::AttachmentLoadOp::eClear) {
            depth_attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        } else {
            depth_attachment.setLoadOp(vk::AttachmentLoadOp::eLoad);
        }
        depth_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);

        rendering_info.setPDepthAttachment(&depth_attachment);
    }

    cmd_().beginRendering(rendering_info);
    in_rendering_ = true;
}

void RenderingThread::draw(u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
    MLE_ASSERT_LOG(in_rendering_ && pipeline_, "Draw can only be called inside rendering with a valid pipeline.");

    if (scissor_.left() > render_area_.width() || scissor_.top() > render_area_.height()) {
        return;
    }

    if (viewport_.width() == 0.0F || viewport_.height() == 0.0F) {
        setViewport();
    }
    if (scissor_.width() == 0 || scissor_.height() == 0) {
        setScissor();
    }

    cmd_().draw(vertex_count, instance_count, first_vertex, first_instance);
}

void RenderingThread::drawIndexed(u32 index_count, u32 instance_count, u32 first_index, int vertex_offset, u32 first_instance) {
    MLE_ASSERT_LOG(in_rendering_ && pipeline_, "DrawIndexed can only be called inside rendering with a valid pipeline.");

    if (scissor_.left() > render_area_.width() || scissor_.top() > render_area_.height()) {
        return;
    }

    if (viewport_.width() == 0.0F || viewport_.height() == 0.0F) {
        setViewport();
    }
    if (scissor_.width() == 0 || scissor_.height() == 0) {
        setScissor();
    }

    cmd_().drawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance);
}

void RenderingThread::dispatchCompute(int group_count_x, int group_count_y, int group_count_z) const {
    MLE_ASSERT_LOG(!in_rendering_ && pipeline_ && pipeline_->isCompute(),
                   "DispatchCompute can only be called outside rendering with a valid compute pipeline.");

    cmd_().dispatch(group_count_x, group_count_y, group_count_z);
}

vk::CommandBuffer RenderingThread::end() {
    MLE_ASSERT_LOG(cmd_.tid() == std::this_thread::get_id(), "RenderingThread::end() called from a different thread.");

    endRendering();
    check(cmd_().end());
    auto ret = cmd_();
    Renderer::i().frameRenderer().releaseSecondaryCommandBuffer(std::move(cmd_));
    return ret;
}
}  // namespace mle
