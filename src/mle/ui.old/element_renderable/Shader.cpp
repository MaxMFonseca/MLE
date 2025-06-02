#include "Shader.h"

namespace mle::ui::element_renderable {
Shader::VertexData Shader::makeVertexData() const {
    return {.pos = element_->getRectOnRoot().pos,
            .size = element_->getRectOnRoot().size,
            .clamp_pos = element_->getRectInternalClamped().pos,
            .clamp_size = element_->getRectInternalClamped().size};
}
}  // namespace mle::ui::element_renderable
