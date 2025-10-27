#pragma once

#include "../Types.h"
#include "mle/ui/shader/ShaderI.h"

namespace mle::ui::comp {
struct Shader {
    std::unique_ptr<ShaderI> impl;

    std::array<std::shared_ptr<ShaderPacketI>, 3> packet_buffers_;

    [[nodiscard]] auto updatePacket(usize idx) const {
        std::shared_ptr<ShaderPacketI> packet = packet_buffers_.at(idx);
        impl->doUpdatePacket(packet.get());
        return packet;
    }

    [[nodiscard]] bool beforeChildren() const { return impl->before_children; }
    [[nodiscard]] bool dedicateRenderTarget() const { return impl->dedicate_render_target; }
    [[nodiscard]] Color clearColor() const { return impl->clear_color; }

    [[nodiscard]] entt::id_type getType() const { return impl->getType(); }

    explicit Shader(std::unique_ptr<ShaderI> impl_) :
        impl(std::move(impl_)) {}
};
}  // namespace mle::ui::comp
