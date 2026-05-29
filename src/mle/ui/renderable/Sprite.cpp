#include "Sprite.h"

#include "../Entt.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Renderable.h"
#include "sol/forward.hpp"

namespace mle::ui::renderable {
namespace {
[[nodiscard]] Expected<vec2u> spriteSourceExtent(const Sprite& sprite) {
    if (sprite.source == SpriteSource::IMAGE) {
        if (sprite.image != nullptr) {
            return sprite.image->getExtent();
        }
        return Renderer::i().textureCache().getDefaultTexture()->getExtent();
    }

    return Renderer::i().textureCache().getExtent(sprite.texture_id);
}

[[nodiscard]] std::optional<vec2f> normalizedFromPixels(const Sprite& sprite, const sol::object& obj) {
    vec2f px{};
    if (!lua::tryAs<vec2f>(obj, px)) {
        return std::nullopt;
    }

    auto extent_r = spriteSourceExtent(sprite);
    if (!extent_r.has_value()) {
        MLE_W("Cannot convert Sprite pixel UVs without a loaded image extent");
        return std::nullopt;
    }

    const vec2f extent = vec2f(extent_r.value());
    if (extent.x == 0.0F || extent.y == 0.0F) {
        MLE_W("Cannot convert Sprite pixel UVs with zero image extent");
        return std::nullopt;
    }

    return px / extent;
}
}  // namespace

void Sprite::removeComponent(const Entt& ew) {
    auto* renderable = ew.tryGet<comp::Renderable>();
    if (!renderable) {
        return;
    }
    if (renderable->impl && renderable->impl->getType() == Sprite::type()) {
        ew.erase<comp::Renderable>();
    }
}

void Sprite::apply(const Entt& e, const sol::object& obj) {
    if (!obj.valid()) {
        removeComponent(e);
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
    source = SpriteSource::TEXTURE;
    image = nullptr;
    texture_id = entt::hashed_string{src.c_str()};
    auto load_r = Renderer::i().textureCache().loadTexture(src);
    if (!load_r.has_value() && load_r.error() != Result::NOT_READY) {
        MLE_E("Failed to load texture {}: {}", src, load_r.error());
        texture_id = {};
    }
    versionUp();
    ew.requestInternalBoundsUpdate();
}

void Sprite::setImage(const Entt& ew, ImageRef image_) {
    source = SpriteSource::IMAGE;
    image = image_;
    texture_id = {};
    versionUp();
    ew.requestInternalBoundsUpdate();
}

void Sprite::setColor(const Color& c) {
    color = c.toLinear();
    versionUp();
};

void Sprite::setFlipX(const sol::object& obj) {
    if (obj.is<bool>()) {
        flip_x = obj.as<bool>();
        versionUp();
    } else {
        MLE_W("Unsupported object type provided to Sprite::setFlipX");
    }
};

void Sprite::setFlipY(const sol::object& obj) {
    if (obj.is<bool>()) {
        flip_y = obj.as<bool>();
        versionUp();
    } else {
        MLE_W("Unsupported object type provided to Sprite::setFlipY");
    }
};

void Sprite::setFit(const sol::object& obj) {
    if (obj.is<bool>()) {
        fit = obj.as<bool>();
        versionUp();
    } else {
        MLE_W("Unsupported object type provided to Sprite::setFit");
    }
};

void Sprite::setUv(const Entt& ew, const sol::object& obj) {
    if (lua::tryAs<vec2f>(obj, uv)) {
        versionUp();
        ew.requestInternalBoundsUpdate();
    } else {
        MLE_W("Unsupported object type provided to Sprite::setUv");
    }
};

void Sprite::setUvSize(const Entt& ew, const sol::object& obj) {
    if (lua::tryAs<vec2f>(obj, uv_size)) {
        versionUp();
        ew.requestInternalBoundsUpdate();
    } else {
        MLE_W("Unsupported object type provided to Sprite::setUvSize");
    }
};

void Sprite::setUvPx(const Entt& ew, const sol::object& obj) {
    if (auto normalized = normalizedFromPixels(*this, obj)) {
        uv = *normalized;
        versionUp();
        ew.requestInternalBoundsUpdate();
    } else {
        MLE_W("Unsupported object type provided to Sprite::setUvPx");
    }
};

void Sprite::setUvSizePx(const Entt& ew, const sol::object& obj) {
    if (auto normalized = normalizedFromPixels(*this, obj)) {
        uv_size = *normalized;
        versionUp();
        ew.requestInternalBoundsUpdate();
    } else {
        MLE_W("Unsupported object type provided to Sprite::setUvSizePx");
    }
};

void Sprite::set(const Entt& ew, const sol::object& obj) {
    MLE_ASSERT(obj.valid());

    if (obj.is<std::string>()) {
        setTexture(ew, obj.as<std::string>());
        return;
    }
    if (obj.is<ImageRef>()) {
        setImage(ew, obj.as<ImageRef>());
        return;
    }
    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        if (const auto image_r = table["image"]; lua::valid<ImageRef>(image_r)) {
            setImage(ew, image_r.get<ImageRef>());
        } else if (const auto texture_r = lua::getFirstKey(table, "texture", 1); lua::valid<std::string>(texture_r)) {
            setTexture(ew, texture_r.as<std::string>());
        }
        if (const sol::object color_r = table["color"]; color_r.valid()) {
            setColor(color_r);
        }
        if (const auto fit_r = table["fit"]; fit_r.valid()) {
            setFit(fit_r);
        }
        if (const auto flip_x_r = table["flip_x"]; flip_x_r.valid()) {
            setFlipX(flip_x_r);
        }
        if (const auto flip_y_r = table["flip_y"]; flip_y_r.valid()) {
            setFlipY(flip_y_r);
        }
        if (const auto uv_r = table["uv"]; uv_r.valid()) {
            setUv(ew, uv_r);
        }
        if (const auto uv_size_r = table["uv_size"]; uv_size_r.valid()) {
            setUvSize(ew, uv_size_r);
        }
        if (const auto uv_px_r = table["uv_px"]; uv_px_r.valid()) {
            setUvPx(ew, uv_px_r);
        }
        if (const auto uv_size_px_r = table["uv_size_px"]; uv_size_px_r.valid()) {
            setUvSizePx(ew, uv_size_px_r);
        }
        return;
    }

    MLE_W("Unsupported object type provided to Sprite::set");
};

const Pipeline* getSpritePipeline() {
    static const Pipeline* pipeline = nullptr;
    if (pipeline == nullptr) {
        Pipeline::CI pipeline_ci{};
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/ui/sprite.vert");
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

void SpritePacket::render(CompRenderingCtx& ctx) {
    if (source_changed) {
        source_changed = false;
        image = nullptr;
    }

    if (!image) {
        if (source == SpriteSource::IMAGE) {
            if (source_image != nullptr) {
                image = source_image;
            } else {
                MLE_W("Sprite image pointer is null, using default texture");
                image = Renderer::i().textureCache().getDefaultTexture();
            }
        } else {
            auto load_r = Renderer::i().textureCache().get(texture_id);
            if (load_r.has_value()) {
                image = load_r.value();
            } else if (load_r.error() != Result::NOT_READY) {
                MLE_E("Failed to get texture id {}: {}", texture_id, load_r.error());
                texture_id = 0;
                image = Renderer::i().textureCache().getDefaultTexture();
            }
        }
    }

    if (!image) {
        return;
    }

    auto& thread = ctx.thread;

    const auto* pipeline = getSpritePipeline();

    thread.setPipeline(pipeline);

    vk::DescriptorImageInfo b0_0_di = image->getDescriptorInfo();
    auto push_writes = pipeline->makeWrites(0, nullptr, &b0_0_di);

    thread.pushDescriptor(0, push_writes);

    struct {
        alignas(4) bool flip_x;
        alignas(4) bool flip_y;
        vec2f uv;
        vec2f uv_size;
        vec2f padding0;
        vec4f color;
        vec4i rounding_corners_radius_px;
        vec2i viewport_size;
    } pc{};

    pc.flip_x = flip_x;
    pc.flip_y = flip_y;
    pc.uv = uv;
    pc.uv_size = uv_size;
    pc.color = color;
    pc.viewport_size = vec2i(ctx.thread.getViewport().size());
    pc.rounding_corners_radius_px = ctx.rounding_corners_radius_px;

    thread.pushConstants(&pc);

    thread.draw(4, 1, 0, 0);
};

[[nodiscard]] vec2u Sprite::calculateBounds([[maybe_unused]] const Entt& e, vec2u max_size) {
    if (fit) {
        return max_size;
    }

    vec2u image_extent{};

    if (source == SpriteSource::IMAGE) {
        image_extent = image != nullptr ? image->getExtent() : Renderer::i().textureCache().getDefaultTexture()->getExtent();
    } else {
        auto& cache = Renderer::i().textureCache();
        auto image_extent_r = cache.getExtent(texture_id);
        image_extent = image_extent_r.has_value() ? image_extent_r.value() : cache.getExtent(0).value();
    }
    vec2f image_extent_f = vec2f(image_extent) * glm::abs(uv_size);

    f32 image_ar = image_extent_f.x / image_extent_f.y;
    f32 max_size_ar = as<f32>(max_size.x) / as<f32>(max_size.y);

    if (max_size_ar > image_ar) {
        max_size.x = as<u32>(as<f32>(max_size.y) * image_ar);
    } else {
        max_size.y = as<u32>(as<f32>(max_size.x) / image_ar);
    }

    return max_size;
};

void Sprite::doUpdatePacket(const Entt& /*ew*/, RenderablePacketI* packet) {
    auto* packet_p = as<SpritePacket*>(packet);
    if (packet_p->source != source || packet_p->source_image != image || packet_p->texture_id != texture_id) {
        packet_p->source = source;
        packet_p->source_image = image;
        packet_p->texture_id = texture_id;
        packet_p->source_changed = true;
    }
    packet_p->color = color;
    packet_p->flip_x = flip_x;
    packet_p->flip_y = flip_y;
    packet_p->uv = uv;
    packet_p->uv_size = uv_size;
};

}  // namespace mle::ui::renderable
