#include "Rendering.h"

#include "../Entt.h"
#include "../UI.h"
#include "mle/core/Logger.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Bounds.h"
#include "mle/ui/components/Renderable.h"
#include "mle/utils/ECS.h"
#include "mle/utils/ID.h"

namespace mle::ui::system {
Rendering::Rendering(UI& ui) :
    ui_(ui) {
    static bool bg_pipeline_created = false;
    if (!bg_pipeline_created) {
        bg_pipeline_created = true;
        Pipeline::CI pipeline_ci{};
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/ui/rect.frag");
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/ui/rect.vert");
        std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::COLOR)};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
        pipeline_ci.blend_attachments = blend_attachments;
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleStrip;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;

        background_pipeline_ = &Renderer::i().pipelineCache().setPipeline("mle_ui_background", pipeline_ci);
    } else {
        background_pipeline_ = &Renderer::i().pipelineCache().getPipeline("mle_ui_background");
    }
    static bool border_pipeline_created = false;
    if (!border_pipeline_created) {
        border_pipeline_created = true;
        Pipeline::CI pipeline_ci{};
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/ui/rect.vert");
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/ui/border.frag");
        std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::COLOR)};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
        pipeline_ci.blend_attachments = blend_attachments;
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleStrip;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;

        border_pipeline_ = &Renderer::i().pipelineCache().setPipeline("mle_ui_border", pipeline_ci);
    } else {
        border_pipeline_ = &Renderer::i().pipelineCache().getPipeline("mle_ui_border");
    }
}

// NOLINTNEXTLINE(misc-no-recursion) Cool recursive function
Rendering::Packet::Node Rendering::createPacketNode(entt::entity entity, usize depth) {
    Entt ew(ui_, entity);

    Packet::Node node;

    node.e_id = entity;

    if (const auto* bounds_r = ew.tryGet<comp::Bounds>(); bounds_r) {
        node.bounds = *bounds_r;
    }
    if (const auto* border_r = ew.tryGet<comp::Border>(); border_r) {
        node.border = *border_r;
    }
    if (const auto* background_r = ew.tryGet<comp::Background>(); background_r) {
        node.bg = background_r->color;
    }
    if (const auto* shader_r = ew.tryGet<comp::Shader>(); shader_r) {
        node.shader = *shader_r;
    }
    if (const auto* renderable_r = ew.tryGet<comp::Renderable>(); renderable_r) {
        node.renderable = renderable_r->clone();
    }

    constexpr usize MAX_DEPTH = 64;
    if (depth > MAX_DEPTH) {
        MLE_E("Exceeded maximum UI node depth of {}. Possible cyclic parent-child relationship. Skiping element {} possible children.", MAX_DEPTH,
              ew.fullName());
        return node;
    }

    auto& self_rel = ew.getRelationship();
    auto children = self_rel.getChildren(ew);
    for (auto child : children) {
        node.children.push_back(createPacketNode(child, depth + 1));
    }

    return node;
};

Rendering::Packet Rendering::createPacket() {
    Packet packet;
    packet.id = next_packet_id_++;
    packet.root = createPacketNode(ui_.getRoot());
    return packet;
};

void Rendering::update() {
    atomic_data_.producerAcquire() = createPacket();

    atomic_data_.producerPublish();
};

// NOLINTNEXTLINE(misc-no-recursion) Cool recursion
void Rendering::renderNode(const Rendering::Packet::Node& node, RenderingContext& ctx) {
    // TODO: use the incoming scissor

    Recti bounds = node.bounds.parent_px;

    Rectf viewport = bounds.asF32();
    viewport.move(ctx.parent_viewport.pos());

    Recti scissor = bounds.constraintTo(ctx.current_scissor);

    if (node.border.color.a > 0) {
        Recti border_bounds = bounds;
        border_bounds.expandTBLR(-node.border.t, node.border.b, -node.border.l, node.border.r);

        Rectf border_viewport = border_bounds.asF32();
        viewport.move(ctx.parent_viewport.pos());

        Recti border_scissor = border_bounds.constraintTo(ctx.current_scissor);

        ctx.rendering_thread.setViewport(border_viewport);
        ctx.rendering_thread.setScissor(border_scissor);

        ctx.rendering_thread.setPipeline(border_pipeline_);

        struct {
            vec4f color;
            vec4i rounging_corners_radius_px;
            vec4i borders_tblr_px;
            vec2i viewport_size;
        } pc{};

        pc.color = node.border.color;
        pc.rounging_corners_radius_px = vec4i{node.border.round_lt, node.border.round_rt, node.border.round_lb, node.border.round_rb};
        pc.borders_tblr_px = vec4i{node.border.t, node.border.b, node.border.l, node.border.r};
        pc.viewport_size = border_bounds.size();

        ctx.rendering_thread.pushConstants(&pc);

        ctx.rendering_thread.draw(4, 1, 0, 0);
    }

    ctx.rendering_thread.setViewport(viewport);
    ctx.rendering_thread.setScissor(scissor);

    if (node.bg.a > 0.0F) {
        ctx.rendering_thread.setPipeline(background_pipeline_);

        struct {
            vec4f color;
            vec4i rounging_corners_radius_px;
            vec2i viewport_size;
        } pc{};

        pc.color = node.bg;
        pc.rounging_corners_radius_px = vec4i{node.border.round_lt, node.border.round_rt, node.border.round_lb, node.border.round_rb};
        pc.viewport_size = node.bounds.parent_px.size();

        ctx.rendering_thread.pushConstants(&pc);

        ctx.rendering_thread.draw(4, 1, 0, 0);
    }

    if (node.renderable) {
        RenderableI::Ctx renderable_ctx{
            .thread = ctx.rendering_thread,
            .viewport_size = bounds.size(),
            .rounding_corners_radius_px = vec4i{node.border.round_lt, node.border.round_rt, node.border.round_lb, node.border.round_rb}};
        node.renderable->render(renderable_ctx);
    }

    for (const auto& child : node.children) {
        renderNode(child, ctx);
    }
};

ImageRef Rendering::getImageForEntity(const Packet::Node& node) {
    auto& images = root_images_[node.e_id];

    auto& frame_renderer = Renderer::i().frameRenderer();
    u32 frame_id = frame_renderer.getCurrentFrameId();

    if (!images.at(frame_id)) {
        Image::CI image_ci{};
        image_ci.extent = node.bounds.parent_px.size();
        image_ci.format = Image::Format::COLOR;

        images.at(frame_id) = Image::createHnd(image_ci);
    } else if (images.at(frame_id)->getExtent() != vec2u(node.bounds.parent_px.size())) {
        frame_renderer.deleteAfterFrame(std::move(images.at(frame_id)));

        Image::CI image_ci{};
        image_ci.extent = node.bounds.parent_px.size();
        image_ci.format = Image::Format::COLOR;

        images.at(frame_id) = Image::createHnd(image_ci);
    }

    return images.at(frame_id).get();
};

ImageRef Rendering::render() {
    if (!atomic_data_.consumerHasNew()) {
        return last_rendered_image_;
    }

    const auto& latest_data = atomic_data_.consumerGetLatest();

    RenderingContext root_ctx;

    AttachmentInfo root_c0{};
    root_c0.image = getImageForEntity(latest_data.root);

    root_c0.image->clear(root_ctx.rendering_thread.cmd());

    root_ctx.rendering_thread.setColorAttachment(root_c0, 0);
    root_ctx.current_scissor.setPos(0, 0);
    root_ctx.current_scissor.setSize(root_c0.image->getExtent());
    root_ctx.parent_viewport = root_ctx.current_scissor;
    root_ctx.rendering_thread.beginRendering();

    renderNode(latest_data.root, root_ctx);

    Renderer::i().frameRenderer().cmd().executeCommands(root_ctx.rendering_thread.end());

    last_rendered_image_ = root_c0.image;
    return last_rendered_image_;
};
}  // namespace mle::ui::system
