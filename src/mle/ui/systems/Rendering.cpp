#include "Rendering.h"

#include "../Entt.h"
#include "../UI.h"
#include "mle/core/Logger.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
#include "mle/ui/Types.h"
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
    lua_.init();
}

void Rendering::clear() {
    Renderer::i().frameRenderer().waitStoped();
    dedicated_images_.clear();
    atomic_data_.getUnsafe(0) = {};
    atomic_data_.getUnsafe(1) = {};
    atomic_data_.getUnsafe(2) = {};
    next_packet_id_ = 0;
}

// NOLINTNEXTLINE(misc-no-recursion) Cool recursive function
Rendering::Packet::Node Rendering::createPacketNode(u8 atomic_buffer_id, entt::entity entity, usize depth) {
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
    if (auto* renderable_r = ew.tryGet<comp::Renderable>(); renderable_r) {
        node.renderable_packet = renderable_r->updatePacket(ew, atomic_buffer_id);
    }
    if (const auto* shader_r = ew.tryGet<comp::Shader>(); shader_r) {
        node.shader_packet = shader_r->updatePacket(atomic_buffer_id);
        node.shader_before_children = shader_r->beforeChildren();
        node.shader_dedicate_render_target = shader_r->dedicateRenderTarget();
        node.shader_clear_color = shader_r->clearColor();
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
        node.children.push_back(createPacketNode(atomic_buffer_id, child, depth + 1));
    }

    return node;
};

void Rendering::update() {
    auto& packet = atomic_data_.producerAcquire();
    packet.id = next_packet_id_++;
    packet.root = createPacketNode(atomic_data_.producerStagingIdx(), ui_.getRoot());
    atomic_data_.producerPublish();
};

void Rendering::renderNodeBorder(const Rendering::Packet::Node& node, RenderingContext& ctx) {
    Recti bounds = node.bounds.parent_px;
    bounds.expandTBLR(-node.border.t, node.border.b, -node.border.l, node.border.r);
    bounds.move(ctx.parent_viewport.pos());
    Rectf viewport = bounds.asF32();
    Recti scissor = bounds.clamp(ctx.parent_scissor);

    ctx.thread.setViewport(viewport);
    ctx.thread.setScissor(scissor);

    ctx.thread.setPipeline(border_pipeline_);

    struct {
        vec4f color;
        vec4i rounging_corners_radius_px;
        vec4i borders_tblr_px;
        vec2i viewport_size;
    } pc{};

    pc.color = node.border.color;
    pc.rounging_corners_radius_px = vec4i{node.border.round_lt, node.border.round_rt, node.border.round_lb, node.border.round_rb};
    pc.borders_tblr_px = vec4i{node.border.t, node.border.b, node.border.l, node.border.r};
    pc.viewport_size = viewport.size();

    ctx.thread.pushConstants(&pc);

    ctx.thread.draw(4, 1, 0, 0);
}

void Rendering::renderNodeBackground(const Rendering::Packet::Node& node, RenderingContext& ctx) {
    Recti bounds = node.bounds.parent_px;
    bounds.move(ctx.parent_viewport.pos());
    Rectf viewport = bounds.asF32();
    Recti scissor = bounds.clamp(ctx.parent_scissor);

    ctx.thread.setViewport(viewport);
    ctx.thread.setScissor(scissor);

    ctx.thread.setPipeline(background_pipeline_);

    struct {
        vec4f color;
        vec4i rounging_corners_radius_px;
        vec2i viewport_size;
    } pc{};

    pc.color = node.bg;
    pc.rounging_corners_radius_px = vec4i{node.border.round_lt, node.border.round_rt, node.border.round_lb, node.border.round_rb};
    pc.viewport_size = viewport.size();

    ctx.thread.pushConstants(&pc);

    ctx.thread.draw(4, 1, 0, 0);
}

ImageRef Rendering::getImageForEntity(const Packet::Node& node) {
    auto& image = dedicated_images_[node.e_id];

    auto& frame_renderer = Renderer::i().frameRenderer();

    if (!image) {
        Image::CI image_ci{};
        image_ci.extent = node.bounds.parent_px.size();
        image_ci.format = Image::Format::COLOR;
        image_ci.extra_usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
        image = Image::createHnd(image_ci);
    } else if (image->getExtent() != vec2u(node.bounds.parent_px.size())) {
        frame_renderer.deleteAfterFrame(std::move(image));

        Image::CI image_ci{};
        image_ci.extent = node.bounds.parent_px.size();
        image_ci.format = Image::Format::COLOR;
        image_ci.extra_usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
        image = Image::createHnd(image_ci);
    }

    return image.get();
};

std::unique_ptr<Rendering::RenderingContext> Rendering::renderCreateNodeNewContext(const Rendering::Packet::Node& node) {
    auto new_ctx = std::make_unique<RenderingContext>();
    new_ctx->thread.init();

    AttachmentInfo color_attachment{};
    color_attachment.image = getImageForEntity(node);

    color_attachment.image->clear(new_ctx->thread.cmd(), node.shader_clear_color);

    new_ctx->thread.setColorAttachment(color_attachment, 0);
    new_ctx->parent_scissor.setPos(0, 0);
    new_ctx->parent_scissor.setSize(color_attachment.image->getExtent());
    new_ctx->parent_viewport = new_ctx->parent_scissor.asF32();
    new_ctx->thread.beginRendering();

    return new_ctx;
}

void Rendering::renderNodeShader(const Rendering::Packet::Node& node, RenderingContext& pctx) {
    CompRenderingCtx shader_packet_ctx{
        .thread = pctx.thread,
        .lua = lua_,
        .rounding_corners_radius_px = vec4i{node.border.round_lt, node.border.round_rt, node.border.round_lb, node.border.round_rb},
    };
    node.shader_packet->render(shader_packet_ctx);
}

void Rendering::renderNodeRenderable(const Rendering::Packet::Node& node, RenderingContext& ctx) {
    CompRenderingCtx renderable_packet_ctx{
        .thread = ctx.thread,
        .lua = lua_,
        .rounding_corners_radius_px = vec4i{node.border.round_lt, node.border.round_rt, node.border.round_lb, node.border.round_rb}};
    node.renderable_packet->render(renderable_packet_ctx);
}

// NOLINTNEXTLINE(misc-no-recursion) Cool recursion
void Rendering::renderNode(const Rendering::Packet::Node& node, RenderingContext& pctx) {
    if (node.bounds.parent_px.left() > pctx.parent_scissor.right() || node.bounds.parent_px.top() > pctx.parent_scissor.bottom()) {
        return;
    }

    if (node.border.color.a > 0) {
        renderNodeBorder(node, pctx);
    }
    if (node.bg.a > 0) {
        renderNodeBackground(node, pctx);
    }

    std::unique_ptr<RenderingContext> new_ctx;
    if (node.shader_packet && node.shader_dedicate_render_target) {
        new_ctx = renderCreateNodeNewContext(node);
    }

    RenderingContext* ctx_r = new_ctx ? new_ctx.get() : &pctx;
    RenderingContext& ctx = *ctx_r;

    Recti bounds = node.bounds.parent_px;
    bounds.move(ctx.parent_viewport.pos());
    Rectf viewport = bounds.asF32();
    Recti scissor = bounds.clamp(ctx.parent_scissor);

    if (node.shader_packet && node.shader_before_children) {
        ctx.thread.setViewport(viewport);
        ctx.thread.setScissor(scissor);
        renderNodeShader(node, pctx);
    }

    if (node.renderable_packet) {
        ctx.thread.setViewport(viewport);
        ctx.thread.setScissor(scissor);
        renderNodeRenderable(node, ctx);
    }

    for (const auto& child : node.children) {
        ctx.parent_viewport = viewport;
        ctx.parent_scissor = scissor;
        renderNode(child, ctx);
    }

    if (node.shader_packet && !node.shader_before_children) {
        ctx.thread.setViewport(viewport);
        ctx.thread.setScissor(scissor);
        renderNodeShader(node, pctx);
    }

    if (new_ctx) {
        ctx.thread.endRendering();
        ctx.thread.executeCommands();

        ImageRef src_color = ctx.thread.getColor0();
        ImageRef dst_color = pctx.thread.getColor0();

        dst_color->copyImage(pctx.thread.cmd(), *src_color, src_color->getExtent(), {0, 0}, bounds.pos());
    }
};

ImageRef Rendering::render() {
    if (!atomic_data_.consumerHasNew()) {
        return last_rendered_image_;
    }

    const auto& latest_data = atomic_data_.consumerGetLatest();

    auto root_ctx = renderCreateNodeNewContext(latest_data.root);

    renderNode(latest_data.root, *root_ctx);

    root_ctx->thread.executeCommands();

    last_rendered_image_ = root_ctx->thread.getColor0();
    return last_rendered_image_;
};
}  // namespace mle::ui::system
