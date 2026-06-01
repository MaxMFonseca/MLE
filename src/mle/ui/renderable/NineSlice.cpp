#include "NineSlice.h"

#include "../Entt.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/components/Renderable.h"
#include "sol/forward.hpp"

namespace mle::ui::renderable {
namespace {
[[nodiscard]] Expected<vec2u> nineSliceSourceExtent(const NineSlice& nine_slice) {
    if (nine_slice.source == NineSliceSource::IMAGE) {
        if (nine_slice.image != nullptr) {
            return nine_slice.image->getExtent();
        }
        return Renderer::i().textureCache().getDefaultTexture()->getExtent();
    }

    return Renderer::i().textureCache().getExtent(nine_slice.texture_id);
}

[[nodiscard]] std::optional<vec2f> normalizedFromPixels(const NineSlice& nine_slice, const sol::object& obj) {
    vec2f px{};
    if (!lua::tryAs<vec2f>(obj, px)) {
        return std::nullopt;
    }

    auto extent_r = nineSliceSourceExtent(nine_slice);
    if (!extent_r.has_value()) {
        MLE_W("Cannot convert NineSlice pixel UVs without a loaded image extent");
        return std::nullopt;
    }

    const vec2f extent = vec2f(extent_r.value());
    if (extent.x == 0.0F || extent.y == 0.0F) {
        MLE_W("Cannot convert NineSlice pixel UVs with zero image extent");
        return std::nullopt;
    }

    return px / extent;
}

[[nodiscard]] int targetBoundToPx(const TargetBound& tb) {
    if (tb.type == TargetBound::Type::PX || tb.type == TargetBound::Type::DEFAULT) {
        return std::max(0, as<int>(std::round(tb.val)));
    }
    MLE_W("NineSlice slice only supports pixel/default units, treating unsupported unit as 0 px");
    return 0;
}

[[nodiscard]] int sliceObjectToPx(const sol::object& obj) {
    TargetBound tb{};
    tb.set(obj);
    return targetBoundToPx(tb);
}

[[nodiscard]] vec2f sourceRegionPx(const NineSlice& nine_slice) {
    auto extent_r = nineSliceSourceExtent(nine_slice);
    if (!extent_r.has_value()) {
        return {1.0F, 1.0F};
    }

    const vec2f extent = vec2f(extent_r.value());
    return glm::max(glm::abs(nine_slice.uv_size) * extent, vec2f{1.0F, 1.0F});
}
}  // namespace

void NineSlice::removeComponent(const Entt& ew) {
    auto* renderable = ew.tryGet<comp::Renderable>();
    if (!renderable) {
        return;
    }
    if (renderable->impl && renderable->impl->getType() == NineSlice::type()) {
        ew.erase<comp::Renderable>();
    }
}

void NineSlice::apply(const Entt& e, const sol::object& obj) {
    if (!obj.valid()) {
        removeComponent(e);
        return;
    }

    auto* renderable = e.tryGet<comp::Renderable>();
    NineSlice* self_p = nullptr;
    if (!renderable) {
        renderable = &e.emplace<comp::Renderable>(std::make_unique<NineSlice>());
        self_p = as<NineSlice*>(renderable->impl.get());
        renderable->packet_buffers_.at(0) = std::make_shared<NineSlicePacket>();
        renderable->packet_buffers_.at(1) = std::make_shared<NineSlicePacket>();
        renderable->packet_buffers_.at(2) = std::make_shared<NineSlicePacket>();
    } else {
        MLE_ASSERT(renderable->impl);
        if (renderable->impl->getType() != NineSlice::type()) {
            MLE_E("Renderable::apply called on entt {} with incompatible Renderable type. {}x{}", e.fullName(), renderable->impl->getType(), NineSlice::type());
            return;
        }
        self_p = as<NineSlice*>(renderable->impl.get());
    }
    self_p->set(e, obj);
};

void NineSlice::setTexture(const Entt& ew, const std::string& src) {
    source = NineSliceSource::TEXTURE;
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

void NineSlice::setImage(const Entt& ew, ImageRef image_) {
    source = NineSliceSource::IMAGE;
    image = image_;
    texture_id = {};
    versionUp();
    ew.requestInternalBoundsUpdate();
}

void NineSlice::setColor(const Color& c) {
    color = c.toLinear();
    versionUp();
};

void NineSlice::setFit(const sol::object& obj) {
    if (obj.is<bool>()) {
        fit = obj.as<bool>();
        versionUp();
    } else {
        MLE_W("Unsupported object type provided to NineSlice::setFit");
    }
};

void NineSlice::setUv(const Entt& ew, const sol::object& obj) {
    if (lua::tryAs<vec2f>(obj, uv)) {
        versionUp();
        ew.requestInternalBoundsUpdate();
    } else {
        MLE_W("Unsupported object type provided to NineSlice::setUv");
    }
};

void NineSlice::setUvSize(const Entt& ew, const sol::object& obj) {
    if (lua::tryAs<vec2f>(obj, uv_size)) {
        versionUp();
        ew.requestInternalBoundsUpdate();
    } else {
        MLE_W("Unsupported object type provided to NineSlice::setUvSize");
    }
};

void NineSlice::setUvPx(const Entt& ew, const sol::object& obj) {
    if (auto normalized = normalizedFromPixels(*this, obj)) {
        uv = *normalized;
        versionUp();
        ew.requestInternalBoundsUpdate();
    } else {
        MLE_W("Unsupported object type provided to NineSlice::setUvPx");
    }
};

void NineSlice::setUvSizePx(const Entt& ew, const sol::object& obj) {
    if (auto normalized = normalizedFromPixels(*this, obj)) {
        uv_size = *normalized;
        versionUp();
        ew.requestInternalBoundsUpdate();
    } else {
        MLE_W("Unsupported object type provided to NineSlice::setUvSizePx");
    }
};

void NineSlice::setSlice(const Entt& ew, const sol::object& obj) {
    if (obj.is<f32>() || obj.is<std::string>()) {
        const int px = sliceObjectToPx(obj);
        slice_px = {.t = px, .b = px, .l = px, .r = px};
        versionUp();
        ew.requestInternalBoundsUpdate();
        return;
    }
    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        if (const auto t_r = lua::getFirstKey(table, "t", 1); t_r.valid()) {
            slice_px.t = sliceObjectToPx(t_r);
        }
        if (const auto b_r = lua::getFirstKey(table, "b", 2); b_r.valid()) {
            slice_px.b = sliceObjectToPx(b_r);
        }
        if (const auto l_r = lua::getFirstKey(table, "l", 3); l_r.valid()) {
            slice_px.l = sliceObjectToPx(l_r);
        }
        if (const auto r_r = lua::getFirstKey(table, "r", 4); r_r.valid()) {
            slice_px.r = sliceObjectToPx(r_r);
        }
        versionUp();
        ew.requestInternalBoundsUpdate();
        return;
    }

    MLE_W("Unsupported object type provided to NineSlice::setSlice");
}

void NineSlice::set(const Entt& ew, const sol::object& obj) {
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
        if (const auto slice_r = table["slice"]; slice_r.valid()) {
            setSlice(ew, slice_r);
        }
        return;
    }

    MLE_W("Unsupported object type provided to NineSlice::set");
};

const Pipeline* getNineSlicePipeline() {
    static const Pipeline* pipeline = nullptr;
    if (pipeline == nullptr) {
        Pipeline::CI pipeline_ci{};
        pipeline_ci.vertex_shader = &Renderer::i().shaderCache().get("mle/ui/nine_slice.vert");
        pipeline_ci.fragment_shader = &Renderer::i().shaderCache().get("mle/ui/nine_slice.frag");
        std::array color_attachment_formats = {Renderer::i().vk().getVkImageFormat(ImageFormat::COLOR)};
        pipeline_ci.color_attachment_formats = color_attachment_formats;
        auto blend_attachments = Pipeline::makeDefaultBlendAttachments<1>();
        pipeline_ci.blend_attachments = blend_attachments;
        pipeline_ci.topology = vk::PrimitiveTopology::eTriangleStrip;
        pipeline_ci.cull_mode = vk::CullModeFlagBits::eNone;
        pipeline_ci.push_descriptor = 0;

        pipeline = &Renderer::i().pipelineCache().setPipeline("mle_ui_nine_slice", pipeline_ci);
    }
    return pipeline;
}

void NineSlicePacket::render(CompRenderingCtx& ctx) {
    if (source_changed) {
        source_changed = false;
        image = nullptr;
    }

    if (!image) {
        if (source == NineSliceSource::IMAGE) {
            if (source_image != nullptr) {
                image = source_image;
            } else {
                MLE_W("NineSlice image pointer is null, using default texture");
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
    const auto* pipeline = getNineSlicePipeline();
    thread.setPipeline(pipeline);

    vk::DescriptorImageInfo b0_0_di = image->getDescriptorInfo();
    auto push_writes = pipeline->makeWrites(0, nullptr, &b0_0_di);
    thread.pushDescriptor(0, push_writes);

    struct {
        vec2f uv;
        vec2f uv_size;
        vec2f source_size_px;
        vec2f padding0;
        vec4i slice_tblr_px;
        vec4f color;
        vec4i rounding_corners_radius_px;
        vec2i viewport_size;
    } pc{};

    pc.uv = uv;
    pc.uv_size = uv_size;
    pc.source_size_px = source_size_px;
    pc.slice_tblr_px = {slice_px.t, slice_px.b, slice_px.l, slice_px.r};
    pc.color = color;
    pc.viewport_size = vec2i(ctx.thread.getViewport().size());
    pc.rounding_corners_radius_px = ctx.rounding_corners_radius_px;

    thread.pushConstants(&pc);
    thread.draw(4, 1, 0, 0);
};

[[nodiscard]] vec2u NineSlice::calculateBounds([[maybe_unused]] const Entt& e, vec2u max_size) {
    if (fit) {
        return max_size;
    }

    vec2f image_extent_f = sourceRegionPx(*this);
    f32 image_ar = image_extent_f.x / image_extent_f.y;
    f32 max_size_ar = as<f32>(max_size.x) / as<f32>(max_size.y);

    if (max_size_ar > image_ar) {
        max_size.x = as<u32>(as<f32>(max_size.y) * image_ar);
    } else {
        max_size.y = as<u32>(as<f32>(max_size.x) / image_ar);
    }

    return max_size;
};

void NineSlice::doUpdatePacket(const Entt& /*ew*/, RenderablePacketI* packet) {
    auto* packet_p = as<NineSlicePacket*>(packet);
    if (packet_p->source != source || packet_p->source_image != image || packet_p->texture_id != texture_id) {
        packet_p->source = source;
        packet_p->source_image = image;
        packet_p->texture_id = texture_id;
        packet_p->source_changed = true;
    }
    packet_p->color = color;
    packet_p->uv = uv;
    packet_p->uv_size = uv_size;
    packet_p->source_size_px = sourceRegionPx(*this);
    packet_p->slice_px = slice_px;
};

}  // namespace mle::ui::renderable
