#include "Renderable.h"

#include "mle/lua/Utils.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/ui/element/Bounds.h"
#include "mle/ui/element/Container.h"

namespace mle::ui::element::comp {
void Renderable::render(RenderContext ctx) const {
    auto& reg = getRegistry();

    renderer::RenderingThreadHnd this_thread = nullptr;

    const auto* root_image = reg.try_get<comp::RootImage>(ctx.self);
    if (root_image) {
        this_thread = std::make_unique<renderer::RenderingThread>();
        this_thread->init();
        renderer::AttachmentInfo attachment;
        attachment.image = root_image->image_handle.get();
        attachment.load = vk::AttachmentLoadOp::eClear;
        attachment.store = vk::AttachmentStoreOp::eStore;
        attachment.clear_value = renderer::toVkClearColor(root_image->clear_color);
        this_thread->setColorAttachments({attachment});
        this_thread->beginRendering();

        ctx.thread = this_thread.get();
    }

    if (i) {
        i->render(ctx);
    } else {
        auto* container = reg.try_get<comp::Container>(ctx.self);
        MLE_ASSERT_LOG(container, "Renderable component without implementation and no Container component found for entity");
        container->render(ctx);
    }

    if (this_thread) {
        this_thread->endRendering();
        this_thread->endCmd();
        addJobToQueue(this_thread->cmd());
    }
}

void Renderable::apply(entt::entity self, const sol::object& obj) const {
    MLE_ASSERT(i);
    i->apply(self, obj);
}

Renderable* Renderable::add(entt::entity e, RenderableImplHnd impl) {
    auto& reg = getRegistry();
    auto* comp = reg.try_get<Renderable>(e);
    MLE_ASSERT_LOG(comp == nullptr, "Renderable component already exists for entity {}", e);
    comp = &reg.emplace<Renderable>(e, std::move(impl));
    return comp;
}

}  // namespace mle::ui::element::comp
