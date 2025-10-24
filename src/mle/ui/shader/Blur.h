#pragma once

#include "mle/ui/shader/ShaderI.h"

namespace mle::ui::shader {
struct BlurPacket : public ShaderPacketI {
    BlurPacket() = default;
    ~BlurPacket() override = default;

    MLE_NO_COPY_MOVE(BlurPacket);

    int radius = 6;
    f32 sigma = 2.0F;

    ImageHnd image;

    void render(CompRenderingCtx& ctx) override;
};

struct Blur : public ShaderI {
    int radius = 6;
    f32 sigma = 2.0F;

    void set(const sol::object& obj) override;

    void doUpdatePacket(ShaderPacketI* packet) override;

    [[nodiscard]] entt::id_type getType() const override { return type(); }
    static entt::id_type type() { return entt::hashed_string{"Blur"}; }

    static void apply(const Entt& e, const sol::object& obj);
};
}  // namespace mle::ui::shader
