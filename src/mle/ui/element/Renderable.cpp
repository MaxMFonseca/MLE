#include "Renderable.h"

#include <vulkan/vulkan_enums.hpp>

#include "mle/lua/Utils.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"
#include "mle/ui/UI.h"

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

        auto bounds = reg.get<Bounds>(ctx.self);

        ctx.current_root_image_bounds = bounds.bounds;
    }

    if (ri_getter_fn) {
        ri_getter_fn(ctx.self).renderComp(ctx);
    }

    if (this_thread) {
        this_thread->endRendering();
        this_thread->endCmd();
        addJobToQueue(this_thread->cmd());
    }
}

Renderable* Renderable::add(entt::entity e, RIGetterFn getter_fn) {
    auto& reg = getRegistry();
    auto* comp = reg.try_get<comp::Renderable>(e);
    if (!comp) {
        comp = &reg.emplace<comp::Renderable>(e);
    }
    comp->ri_getter_fn = getter_fn;
    return comp;
}

void RootImage::lkh(entt::entity self, const sol::object& o) {
    lua::assertIs<sol::table>(o);
    auto& reg = getRegistry();

    auto table = o.as<sol::table>();

    auto& comp = reg.emplace_or_replace<RootImage>(self);
    renderer::Image::CI ci;
    ci.usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment;
    ci.extent = lua::as<vec2i>(table["extent"]);
    ci.format = renderer::getDefaultColorFormat();
    comp.image_handle = renderer::Image::createHnd(ci);
    comp.clear_color = Color::fromLua(table["clear_color"]);
}
}  // namespace mle::ui::element::comp
