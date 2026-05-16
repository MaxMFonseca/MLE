#include "RenderImage.h"

#include "mle/lua/Utils.h"
#include "mle/renderer/Pipeline.h"
#include "mle/ui/Entt.h"
#include "mle/ui/components/Renderable.h"
#include "mle/ui/renderable/Sprite.h"
#include "sol/forward.hpp"

namespace mle::ui::renderable {
void RenderImage::apply(const Entt& e, const sol::object& obj) {
    if (!obj.valid()) {
        MLE_E("Invalid object provided to RenderImage::apply for entt {}", e.e());
        return;
    }
    auto* renderable = e.tryGet<comp::Renderable>();
    RenderImage* self_p = nullptr;
    if (!renderable) {
        renderable = &e.emplace<comp::Renderable>(std::make_unique<RenderImage>());
        self_p = as<RenderImage*>(renderable->impl.get());
        renderable->packet_buffers_.at(0) = std::make_shared<RenderImagePacket>();
        renderable->packet_buffers_.at(1) = std::make_shared<RenderImagePacket>();
        renderable->packet_buffers_.at(2) = std::make_shared<RenderImagePacket>();
    } else {
        MLE_ASSERT(renderable->impl);
        if (renderable->impl->getType() != RenderImage::type()) {
            MLE_E("Renderable::apply called on entt {} with incompatible Renderable type. {}x{}", e.fullName(), renderable->impl->getType(),
                  RenderImage::type());
            return;
        }
        self_p = as<RenderImage*>(renderable->impl.get());
    }
    self_p->set(e, obj);
};

void RenderImage::setColor(const Color& c) {
    color = c.toLinear();
    versionUp();
};

void RenderImage::setFlipX(const sol::object& obj) {
    if (obj.is<bool>()) {
        flip_x = obj.as<bool>();
        versionUp();
    } else {
        MLE_W("Unsupported object type provided to RenderImage::setFlipX");
    }
};

void RenderImage::setFlipY(const sol::object& obj) {
    if (obj.is<bool>()) {
        flip_y = obj.as<bool>();
        versionUp();
    } else {
        MLE_W("Unsupported object type provided to RenderImage::setFlipY");
    }
};

void RenderImage::set(const Entt& /*e*/, const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<bool>()) {
        versionUp();
        return;
    }
    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        if (const sol::object color_r = table["color"]; color_r.valid()) {
            setColor(color_r);
        }
        if (const auto flip_x_r = table["flip_x"]; flip_x_r.valid()) {
            setFlipX(flip_x_r);
        }
        if (const auto flip_y_r = table["flip_y"]; flip_y_r.valid()) {
            setFlipY(flip_y_r);
        }
        return;
    }

    MLE_W("Unsupported object type provided to RenderImage::set");
};

void RenderImagePacket::render(CompRenderingCtx& ctx) {
    if (!ctx.dedicated_image) {
        return;
    }

    auto& thread = ctx.thread;

    const auto* pipeline = getSpritePipeline();

    thread.setPipeline(pipeline);

    vk::DescriptorImageInfo b0_0_di = ctx.dedicated_image->getDescriptorInfo();
    auto push_writes = pipeline->makeWrites(0, nullptr, &b0_0_di);

    thread.pushDescriptor(0, push_writes);

    struct {
        alignas(4) bool flip_x;
        alignas(4) bool flip_y;
        vec2f padding0;
        vec4f color;
        vec4i rounding_corners_radius_px;
        vec2i viewport_size;
    } pc{};

    pc.flip_x = flip_x;
    pc.flip_y = flip_y;
    pc.color = color;
    pc.viewport_size = vec2i(ctx.thread.getViewport().size());
    pc.rounding_corners_radius_px = ctx.rounding_corners_radius_px;

    thread.pushConstants(&pc);

    thread.draw(4, 1, 0, 0);
};

[[nodiscard]] vec2u RenderImage::calculateBounds([[maybe_unused]] const Entt& e, vec2u max_size) {
    return max_size;
};

void RenderImage::doUpdatePacket(const Entt& /*ew*/, RenderablePacketI* packet) {
    auto* packet_p = as<RenderImagePacket*>(packet);
    packet_p->color = color;
    packet_p->flip_x = flip_x;
    packet_p->flip_y = flip_y;
};
}  // namespace mle::ui::renderable
