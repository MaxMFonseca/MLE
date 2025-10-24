#pragma once

#include "../Types.h"
#include "mle/ui/shader/ShaderI.h"

namespace mle::ui::comp {
struct Shader {
    std::unique_ptr<ShaderI> impl;

    bool before_children = false;
    bool dedicate_render_target = false;
    Color clear_color = Color::ZERO;

    std::array<std::shared_ptr<ShaderPacketI>, 3> packet_buffers_;

    [[nodiscard]] auto updatePacket(usize idx) const {
        std::shared_ptr<ShaderPacketI> packet = packet_buffers_.at(idx);
        impl->doUpdatePacket(packet.get());
        return packet;
    }

    explicit Shader(std::unique_ptr<ShaderI> impl_) :
        impl(std::move(impl_)) {}
};
}  // namespace mle::ui::comp
