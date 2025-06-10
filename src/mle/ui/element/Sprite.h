#pragma once

#include "Renderable.h"

namespace mle::ui::element {
struct Sprite : RenderableInterface {
    void render(renderer::RenderingThread& thread) const override;
};
}  // namespace mle::ui::element
