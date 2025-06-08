#include "ElementManager.h"

#include "Element.h"
#include "mle/lua/Utils.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Utils.h"
#include "mle/ui/detail/Container.h"

namespace mle::ui::detail {
void ElementManager::init() {
    MLE_I("Initializing ElementManager");
    addLuaKeysEngine();

    renderer::Pipeline::CI pipeline_ci;
    pipeline_ci.vertex_shader = renderer::getShader("triangle.vert", true);
    pipeline_ci.fragment_shader = renderer::getShader("triangle.frag", true);
    pipeline_ci.color_attachment_formats.push_back(renderer::getDefaultColorFormat());
    pipeline_ci.blend_attachments = renderer::makeDefaultBlendAttachmentStates(1);
    pipeline_ = renderer::Pipeline::createHnd(pipeline_ci);
}

void ElementManager::shutdown() {
    MLE_I("Shutting down ElementManager");
    registry_.clear();
}

void ElementManager::resetRoot(const sol::table& table) {
    MLE_D("Resetting ElementManager");
    registry_.clear();

    if (!table.valid()) {
        MLE_I("ElementManager reset called with no table");
        return;
    }

    MLE_D("Root element: {}", table);

    root_ = registry_.create();
    MLE_ASSERT_LOG(table["flex"].valid(), "Root element must have a 'flex' key");
    e_components::FlexContainer::onLuaKeyFlex(registry_, root_, table["flex"]);
    auto image_table = lua::getKey<sol::table>(table, "image");
    registry_.emplace<e_components::RootImage>(root_, image_table);
}

renderer::ImageRef ElementManager::render() {
    auto& root_image = registry_.get<e_components::RootImage>(root_);

    renderer::RenderingThread thread;
    thread.init(true);
    renderer::AttachmentInfo color_attachment;
    color_attachment.image = root_image.getImage();
    color_attachment.load = vk::AttachmentLoadOp::eClear;
    color_attachment.store = vk::AttachmentStoreOp::eStore;
    color_attachment.clear_value.color = root_image.getClearColorVk();

    renderer::RenderingInfo rendering_info;
    rendering_info.pipeline = pipeline_.get();
    rendering_info.colors.push_back(color_attachment);
    thread.beginRendering(rendering_info);
    thread.setViewport();
    thread.draw(1, 3);
    thread.end();

    registry_.get<e_components::Children>(root_).render();

    return root_image.getImage();
}

void ElementManager::addLuaKeysEngine() {
    lua_keys_["name"] = e_components::Name::onLuaKeyName;
    lua_keys_["flex"] = e_components::FlexContainer::onLuaKeyFlex;
}

Expected<ElementManager::LuaKeyFunction> ElementManager::findLuaKey(const std::string& key) {
    auto it = lua_keys_.find(key);
    if (it != lua_keys_.end()) {
        return it->second;
    }
    MLE_W("Lua key '{}' not found", key);
    return std::unexpected(Result::NOT_FOUND);
}
}  // namespace mle::ui::detail
