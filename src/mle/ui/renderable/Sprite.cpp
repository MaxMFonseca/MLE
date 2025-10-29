#include "Sprite.h"

#include "../Entt.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Renderable.h"
#include "sol/forward.hpp"

namespace mle::ui::renderable {
void Sprite::apply(const Entt& e, const sol::object& obj) {
    if (!obj.valid()) {
        MLE_E("Invalid object provided to Sprite::apply for entt {}", e.e());
        return;
    }
    auto* renderable = e.tryGet<comp::Renderable>();
    Sprite* self_p = nullptr;
    if (!renderable) {
        renderable = &e.emplace<comp::Renderable>(std::make_unique<Sprite>());
        self_p = as<Sprite*>(renderable->impl.get());
        renderable->packet_buffers_.at(0) = std::make_shared<SpritePacket>();
        renderable->packet_buffers_.at(1) = std::make_shared<SpritePacket>();
        renderable->packet_buffers_.at(2) = std::make_shared<SpritePacket>();
    } else {
        MLE_ASSERT(renderable->impl);
        if (renderable->impl->getType() != Sprite::type()) {
            MLE_E("Renderable::apply called on entt {} with incompatible Renderable type. {}x{}", e.fullName(), renderable->impl->getType(), Sprite::type());
            return;
        }
        self_p = as<Sprite*>(renderable->impl.get());
    }
    self_p->set(e, obj);
};

void Sprite::setTexture(const Entt& ew, const std::string& src) {
    texture_id = entt::hashed_string{src.c_str()};
    auto load_r = Renderer::i().textureCache().loadTexture(src);
    if (load_r.error() != Result::NOT_READY) {
        MLE_E("Failed to load texture {}: {}", src, load_r.error());
        texture_id = {};
    }
    ew.requestInternalBoundsUpdate();
}

void Sprite::setColor(const Color& c) {
    color = c;
    versionUp();
};

void Sprite::set(const Entt& ew, const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<std::string>()) {
        setTexture(ew, obj.as<std::string>());
        return;
    }
    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        if (const auto texture_r = lua::getFirstKey(table, "texture", 1); lua::valid<std::string>(texture_r)) {
            setTexture(ew, texture_r.as<std::string>());
        }
        if (const sol::object color_r = table["color"]; color_r.valid()) {
            setColor(color_r);
        }
        return;
    }

    MLE_W("Unsupported object type provided to Sprite::set");
};

namespace {
auto getPipeline() {
    static const Pipeline* pipeline = nullptr;
    if (pipeline == nullptr) {
        Pipeline::CI pipeline_ci{};
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/ui/rect.vert");
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/ui/sprite.frag");
        std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::COLOR)};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
        pipeline_ci.blend_attachments = blend_attachments;
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleStrip;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        pipeline_ci.push_descriptor = 0;

        pipeline = &Renderer::i().pipelineCache().setPipeline("mle_ui_sprite", pipeline_ci);
    }
    return pipeline;
}
}  // namespace

void SpritePacket::render(CompRenderingCtx& ctx) {
    if (!image || texture_id_changed) {
        texture_id_changed = false;
        auto load_r = Renderer::i().textureCache().get(texture_id);
        if (load_r.has_value()) {
            image = load_r.value();
        } else if (load_r.error() != Result::NOT_READY) {
            MLE_E("Failed to get texture id {}: {}", texture_id, load_r.error());
            texture_id = 0;
            image = Renderer::i().textureCache().getDefaultTexture();
        }
    }

    auto& thread = ctx.thread;

    const auto* pipeline = getPipeline();

    thread.setPipeline(pipeline);

    vk::DescriptorImageInfo b0_0_di = image->getDescriptorInfo();
    auto push_writes = pipeline->makeWrites(0, nullptr, &b0_0_di);

    thread.pushDescriptor(0, push_writes);

    struct {
        vec4f color;
        vec4i rounding_corners_radius_px;
        vec2i viewport_size;
    } pc{};

    pc.color = color;
    pc.viewport_size = vec2i(ctx.thread.getViewport().size());
    pc.rounding_corners_radius_px = ctx.rounding_corners_radius_px;

    thread.pushConstants(&pc);

    thread.draw(4, 1, 0, 0);
};

[[nodiscard]] vec2u Sprite::calculateBounds([[maybe_unused]] const Entt& e, vec2u max_size) {
    vec2u image_extent{};

    auto& cache = Renderer::i().textureCache();
    auto image_extent_r = cache.getExtent(texture_id);
    image_extent = image_extent_r.has_value() ? image_extent_r.value() : cache.getExtent(0).value();
    vec2f image_extent_f = image_extent;

    f32 image_ar = image_extent_f.x / image_extent_f.y;
    f32 max_size_ar = as<f32>(max_size.x) / as<f32>(max_size.y);

    if (max_size_ar > image_ar) {
        max_size.x = as<u32>(as<f32>(max_size.y) * image_ar);
    } else {
        max_size.y = as<u32>(as<f32>(max_size.x) / image_ar);
    }

    if (image_extent.x <= max_size.x && image_extent.y <= max_size.y) {
        return image_extent;
    }

    return max_size;
};

void Sprite::doUpdatePacket(const Entt& /*ew*/, RenderablePacketI* packet) {
    auto* packet_p = as<SpritePacket*>(packet);
    if (packet_p->texture_id != texture_id) {
        packet_p->texture_id = texture_id;
        packet_p->texture_id_changed = true;
    }
    packet_p->color = color;
};

}  // namespace mle::ui::renderable
