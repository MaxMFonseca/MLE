#include "Blit.h"

#include "mle/renderer/RenderingThread.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/Renderable.h"

namespace mle::ui::element {
Blit::Blit(renderer::ImageRef image) :
    image_(image) {
}

void Blit::apply(entt::entity /*unused*/, const sol::object& /*unused*/) {
    MLE_UNREACHABLE_LOG("Blit::apply should not be called, Blit is a simple renderable that does not have any properties to apply");
}

void Blit::render(const RenderContext& ctx) const {
    ctx.thread->endRendering();

    ctx.thread->getColor0Image()->updateBlit(ctx.thread->cmd(), image_, {}, ctx.thread->getRenderArea());

    ctx.thread->beginRendering({}, false);
}
}  // namespace mle::ui::element
