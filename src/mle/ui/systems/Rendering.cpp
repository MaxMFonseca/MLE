#include "Rendering.h"

#include "../Entt.h"
#include "../UI.h"
#include "mle/core/Logger.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
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

    auto children = comp::Container::getChildren(ew);
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
    MLE_E("Rendering node with bounds: {}, boder: {}, bg: {}", node.bounds, node.border, node.bg);

    Rectf bounds_f = node.bounds.parent_px.asF32();
    Rectf viewport = ctx.current_viewport;
    viewport.move(bounds_f.pos());
    vec2f max_size = viewport.size() - bounds_f.pos();
    viewport.setWidth(std::min(bounds_f.width(), max_size.x));
    viewport.setHeight(std::min(bounds_f.height(), max_size.y));

    ctx.rendering_thread.setViewportAndScissor(viewport);

    if (node.bg.a > 0.0F) {
        ctx.rendering_thread.setPipeline(background_pipeline_);

        struct PushConstants {
            vec4f color;
        };

        PushConstants pc{.color = node.bg};

        ctx.rendering_thread.pushConstants(&pc);

        ctx.rendering_thread.draw(4, 1, 0, 0);
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
        MLE_I("No new rendering data to render. Using latest rendered image.");
        return last_rendered_image_;
    }

    const auto& latest_data = atomic_data_.consumerGetLatest();

    RenderingContext root_ctx;

    AttachmentInfo root_c0{};
    root_c0.image = getImageForEntity(latest_data.root);

    root_c0.image->clear(root_ctx.rendering_thread.cmd());

    root_ctx.rendering_thread.setColorAttachment(root_c0, 0);
    root_ctx.current_viewport = Rectf{0, 0, as<f32>(root_c0.image->getExtent().x), as<f32>(root_c0.image->getExtent().y)};
    root_ctx.rendering_thread.beginRendering();

    renderNode(latest_data.root, root_ctx);

    Renderer::i().frameRenderer().cmd().executeCommands(root_ctx.rendering_thread.end());

    last_rendered_image_ = root_c0.image;
    return last_rendered_image_;
};
}  // namespace mle::ui::system
