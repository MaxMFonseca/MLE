#pragma once

#include "mle/ui/components/Bounds.h"
#include "mle/ui/renderable/RenderableI.h"
#include "mle/utils/ECS.h"

namespace mle::ui::renderable {
enum class NineSliceSource : u8 { TEXTURE, IMAGE };

[[nodiscard]] const Pipeline* getNineSlicePipeline();

struct NineSlicePacket : public RenderablePacketI {
    NineSlicePacket() = default;
    ~NineSlicePacket() override = default;

    MLE_NO_COPY_MOVE(NineSlicePacket);

    ImageRef image{};
    Color color = Color::ONE;
    NineSliceSource source = NineSliceSource::TEXTURE;
    ImageRef source_image{};
    entt::id_type texture_id{};
    bool source_changed = false;
    vec2f uv = {0.0F, 0.0F};
    vec2f uv_size = {1.0F, 1.0F};
    vec2f source_size_px = {1.0F, 1.0F};
    PaddingPx slice_px{};

    void render(CompRenderingCtx& ctx) override;
};

struct NineSlice : public RenderableI {
    NineSliceSource source = NineSliceSource::TEXTURE;
    ImageRef image{};
    entt::id_type texture_id{};
    Color color = Color::ONE;
    vec2f uv = {0.0F, 0.0F};
    vec2f uv_size = {1.0F, 1.0F};
    PaddingPx slice_px{};

    bool fit = false;

    void setTexture(const Entt& ew, const std::string& src);
    void setImage(const Entt& ew, ImageRef image);
    void setColor(const Color& c);
    void setColor(const sol::object& obj) { setColor(Color::fromLua(obj)); }
    void setFit(const sol::object& obj);
    void setUv(const Entt& ew, const sol::object& obj);
    void setUvSize(const Entt& ew, const sol::object& obj);
    void setUvPx(const Entt& ew, const sol::object& obj);
    void setUvSizePx(const Entt& ew, const sol::object& obj);
    void setSlice(const Entt& ew, const sol::object& obj);

    void set(const Entt& e, const sol::object& obj) override;
    [[nodiscard]] vec2u calculateBounds(const Entt& e, vec2u max_size) override;

    [[nodiscard]] entt::id_type getType() const override { return type(); }
    static entt::id_type type() { return entt::hashed_string{"NineSlice"}; }

    void doUpdatePacket(const Entt& ew, RenderablePacketI* packet) override;

    static void apply(const Entt& e, const sol::object& obj);

    static void removeComponent(const Entt& ew);
};
}  // namespace mle::ui::renderable
