#include "RenderingThread.h"

#include <vulkan/vulkan_structs.hpp>

#include "Pipeline.h"
#include "Renderer.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"
#include "mle/renderer/detail/FrameRenderer.h"
namespace mle::renderer {
void RenderingThread::init(bool primary) {
    if (primary) {
        MLE_I("Initializing primary rendering thread...");
        cmd_ = detail::getFrameRenderer().getCmd();
        primary_ = true;
    } else {
        MLE_I("Initializing secondary rendering thread...");
        cmd_ = detail::getFrameRenderer().getCmdSecondary();
        vk::CommandBufferBeginInfo cmd_begin_info{};
        cmd_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        check(cmd_.begin(cmd_begin_info));
    }
}

void RenderingThread::end() {
    if (rendering_data_.pipeline) {
        cmd_.endRendering();
    }

    if (!primary_) {
        check(cmd_.end());
    }
}

void RenderingThread::beginRendering(const RenderingInfo& info) {
    auto render_area = info.render_area;

    if (info.render_area.size.x == 0) {
        render_area.size.x = info.colors.at(0).image->getExtent().x - info.render_area.pos.x;
    }
    if (info.render_area.size.y == 0) {
        render_area.size.y = info.colors.at(0).image->getExtent().y - info.render_area.pos.y;
    }

    vk::RenderingInfo render_info;
    render_info.setRenderArea({
        {render_area.pos.x, render_area.pos.y},
        {static_cast<u32>(render_area.size.x), static_cast<u32>(render_area.size.y)},
    });
    render_info.setViewMask(0);
    render_info.setLayerCount(1);

    std::vector<vk::RenderingAttachmentInfo> attachment_descriptions{info.colors.size()};
    for (usize i = 0; i < info.colors.size(); i++) {
        const auto& attachment = info.colors.at(i);
        auto& rattachment = attachment_descriptions.at(i);

        attachment.image->transitionState(cmd_, Image::State::COLOR_ATT);

        if (attachment.view) {
            rattachment.imageView = attachment.view;
        } else {
            rattachment.imageView = attachment.image->getDefaultView();
        }
        rattachment.loadOp = attachment.load;
        rattachment.clearValue = attachment.clear_value;
        rattachment.storeOp = attachment.store;
        rattachment.imageLayout = vk::ImageLayout::eAttachmentOptimal;
    }
    render_info.setColorAttachments(attachment_descriptions);

    vk::RenderingAttachmentInfo depth_attachment;
    if (info.depth.image || info.depth.view) {
        info.depth.image->transitionState(cmd_, Image::State::DEPTH_ATT);

        depth_attachment.imageView = !info.depth.view ? info.depth.image->getDefaultView() : info.depth.view;
        depth_attachment.loadOp = info.depth.load;
        depth_attachment.clearValue = info.depth.clear_value;
        depth_attachment.storeOp = info.depth.store;
        depth_attachment.imageLayout = vk::ImageLayout::eAttachmentOptimal;

        render_info.setPDepthAttachment(&depth_attachment);
    }

    if (rendering_data_.pipeline) {
        cmd_.endRendering();
    }

    cmd_.beginRendering(render_info);

    rendering_data_.previous_pipeline = rendering_data_.pipeline;
    rendering_data_.pipeline = info.pipeline;
    rendering_data_.vk_rendering = render_info;
}

void RenderingThread::setViewport(const Rectf& viewport) const {
    MLE_ASSERT_LOG(viewport.size.x >= 0.0F && viewport.size.y >= 0.0F, "Viewport size must be non-negative");
    MLE_ASSERT_LOG(viewport.pos.x >= 0.0F && viewport.pos.y >= 0.0F, "Viewport position must be non-negative");
    MLE_T("Setting viewport: pos = {}, size = {}", viewport.pos, viewport.size);

    vk::Viewport vv{};
    vv.x = viewport.pos.x;
    vv.y = viewport.pos.y;
    vv.minDepth = 0.0F;
    vv.maxDepth = 1.0F;

    if (viewport.size.x == 0.0F) {
        vv.width = static_cast<f32>(getCurrentRenderArea().extent.width) - viewport.pos.x;
    } else {
        vv.width = viewport.size.x;
    }

    if (viewport.size.y == 0.0F) {
        vv.height = static_cast<f32>(getCurrentRenderArea().extent.height) - viewport.pos.y;
    } else {
        vv.height = viewport.size.y;
    }

    vk::Rect2D scissor{};
    scissor.offset.x = static_cast<int>(viewport.pos.x);
    scissor.offset.y = static_cast<int>(viewport.pos.y);
    scissor.extent.width = static_cast<u32>(vv.width);
    scissor.extent.height = static_cast<u32>(vv.height);

    cmd_.setViewport(0, {vv});
    cmd_.setScissor(0, scissor);
}

void RenderingThread::bindVertexBuffer(BufferRef buffer, usize offset) const {
    cmd_.bindVertexBuffers(0, buffer->get(), offset);
}

void RenderingThread::bindInstanceBuffer(BufferRef buffer, usize offset) const {
    cmd_.bindVertexBuffers(1, buffer->get(), offset);
}

void RenderingThread::bindIndexBuffer(BufferRef buffer, usize offset) const {
    cmd_.bindIndexBuffer(buffer->get(), offset, vk::IndexType::eUint32);
}

void RenderingThread::pushConstants(const void* push_constants) {
    MLE_ASSERT(rendering_data_.pipeline->hasPushConstants());
    auto pc_size = rendering_data_.pipeline->getPushConstantSize();
    auto pc_frag_offset = rendering_data_.pipeline->getPushConstantFragOffset();
    auto pipeline_layout = rendering_data_.pipeline->getPipelineLayout();

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

void RenderingThread::draw(int instance_count, int index_count) const {
    if (rendering_data_.pipeline != rendering_data_.previous_pipeline) {
        cmd_.bindPipeline(vk::PipelineBindPoint::eGraphics, rendering_data_.pipeline->get());
    }

    cmd_.draw(index_count, instance_count, 0, 0);
}
}  // namespace mle::renderer
