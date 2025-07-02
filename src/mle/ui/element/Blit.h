#pragma once

#include "mle/common/Utils.h"
#include "mle/renderer/Types.h"
#include "mle/ui/element/Renderable.h"

namespace mle::ui::element {
class Blit : public RenderableImpl {
  public:
    MLE_NO_COPY_MOVE(Blit)

    explicit Blit(renderer::ImageRef image);
    ~Blit() override = default;

    void render(const RenderContext& ctx) const override;

    [[nodiscard]] vec2i getSize() const override { return image_->getExtent(); }

  private:
    renderer::ImageRef image_;
};
}  // namespace mle::ui::element
