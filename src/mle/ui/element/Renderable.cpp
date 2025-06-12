#include "Renderable.h"

#include <vulkan/vulkan_enums.hpp>

#include "mle/lua/Utils.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"
#include "mle/ui/UI.h"

namespace mle::ui::element::comp {
void Renderable::render(entt::entity self, renderer::RenderingThreadRef thread) const {
    auto& reg = getRegistry();

    renderer::RenderingThreadHnd this_thread = nullptr;
    const auto* root_image = reg.try_get<comp::RootImage>(self);
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

        thread = this_thread.get();
    }

    ri.renderComp(self, thread);

    if (this_thread) {
        this_thread->submit();
    }
}

void Renderable::add(entt::entity e, RenderableInterface& ri) {
    auto& reg = getRegistry();
    MLE_ASSERT(!reg.any_of<comp::Renderable>(e));
    reg.emplace<comp::Renderable>(e, ri);
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
