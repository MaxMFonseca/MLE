#pragma once

#include "../Types.h"
#include "mle/renderer/RenderingThread.h"

namespace mle::ui::element {
struct RenderableInterface {
    MLE_NO_COPY_MOVE(RenderableInterface)

    virtual ~RenderableInterface() = default;

    virtual void render(renderer::RenderingThread& thread) const = 0;
};

struct Renderable {
    RenderableInterface* impl = nullptr;
};
}  // namespace mle::ui::element
