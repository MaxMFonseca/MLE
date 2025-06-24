#include "UI.h"

#include <memory>
#include <vulkan/vulkan_handles.hpp>

#include "mle/common/Assert.h"
#include "mle/common/RectPacker.h"
#include "mle/common/Utils.h"
#include "mle/lua/Lua.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"
#include "mle/ui/element/Base.h"
#include "mle/ui/element/Collidable.h"
#include "mle/ui/element/Container.h"
#include "mle/ui/element/LuaKeyHandlers.h"
#include "mle/window/Window.h"

namespace mle::ui {
namespace {
class Impl {
  public:
    inline void init();
    inline void shutdown();

    inline void update();
    inline renderer::ImageRef render();

    auto& getRegistry() { return registry_; }
    auto& getFontCache() { return font_cache_; }
    [[nodiscard]] auto getPreRenderCmdBuffer() const { return pre_render_cmd_buffer_; }
    void addJobToQueue(vk::CommandBuffer cmd) { s_command_buffers_.emplace_back(cmd); }
    [[nodiscard]] auto getRootSize() const { return root_size_; }

  private:
    void checkElementsBoundChanged();

  private:
    entt::registry registry_;
    entt::entity root_ = entt::null;
    detail::FontCache font_cache_;

    vk::CommandBuffer pre_render_cmd_buffer_;
    std::vector<vk::CommandBuffer> s_command_buffers_;

    vec2u root_size_{0};
};

struct Position {
    vec2f pos;
};

void Impl::init() {
    MLE_I("Initializing UI");

    root_size_ = window::getSize();

    renderer::addTexture("mle", res::addMleTexturePath("ui/mle.png"));

    element::addEngineLuaKeyHandlers();

    font_cache_.init();

    Font::CI default_font_ci;
    default_font_ci.name = "DigitalDisco";
    default_font_ci.engine = true;
    default_font_ci.pre_load_string = U"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;':\",.<>?/~`";
    auto& font = *font_cache_.add(std::move(default_font_ci));
    MLE_VD(font);

    root_ = registry_.create();
    element::applyEntityTable(root_, lua::require("testui", true));

    MLE_ASSERT_LOG(registry_.try_get<element::comp::RootImage>(root_), "The root element must have the root_image field set.");
}

void Impl::shutdown() {
    MLE_I("Shutting down UI");

    font_cache_.reset();
}

void Impl::update() {
    vec2u cursor = window::getUIM().getCursorPos();
    auto& root_container = registry_.get<element::comp::Container>(root_);
    root_container.collide(root_, cursor);

    auto view = registry_.view<element::comp::Collidable>();
    for (auto e : view) {
        view.get<element::comp::Collidable>(e).update(e);
    }

    // updateElements();

    checkElementsBoundChanged();
}

void Impl::checkElementsBoundChanged() {
    auto view = registry_.view<element::comp::ChildChangedBounds>();
    if (view->empty()) {
        return;
    }

    if (view->size() == 1) {
        auto e = *view.begin();
        registry_.get<element::comp::Container>(e).updateChildrenBounds(e, root_size_);
    } else {
        registry_.get<element::comp::Container>(root_).updateChildrenBounds(root_, root_size_, true);
    }

    registry_.clear<element::comp::ChildChangedBounds>();
}

renderer::ImageRef Impl::render() {
    pre_render_cmd_buffer_ = renderer::getFrameSecondaryCmd();
    vk::CommandBufferBeginInfo beg_info;
    beg_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    vk::CommandBufferInheritanceInfo ihi{};
    beg_info.setPInheritanceInfo(&ihi);
    renderer::check(pre_render_cmd_buffer_.begin(beg_info));

    registry_.get<element::comp::Renderable>(root_).render({.self = root_});

    renderer::check(pre_render_cmd_buffer_.end());
    renderer::submitJobOnFrame(pre_render_cmd_buffer_);
    for (auto cmd : s_command_buffers_) {
        renderer::submitJobOnFrame(cmd);
    }
    s_command_buffers_.clear();

    return registry_.get<element::comp::RootImage>(root_).image_handle.get();
}

std::unique_ptr<Impl> i_ = nullptr;  // NOLINT
}  // namespace

void init() {
    MLE_ASSERT(!i_);
    i_ = std::make_unique<Impl>();
    i_->init();
}

void shutdown() {
    MLE_ASSERT(i_);
    i_->shutdown();
    i_.reset();
}

void update() {
    MLE_ASSERT(i_);
    i_->update();
}

renderer::ImageRef render() {
    MLE_ASSERT(i_);
    return i_->render();
}

entt::registry& getRegistry() {
    MLE_ASSERT(i_);
    return i_->getRegistry();
}

FontRef getFont(const std::string& font_name) {
    MLE_ASSERT(i_);
    return i_->getFontCache().get(font_name);
}

vk::CommandBuffer getPreRenderCmdBuffer() {
    MLE_ASSERT(i_);
    return i_->getPreRenderCmdBuffer();
}

void addJobToQueue(vk::CommandBuffer cmd) {
    MLE_ASSERT(i_);
    i_->addJobToQueue(cmd);
}

vec2u getRootSize() {
    MLE_ASSERT(i_);
    return i_->getRootSize();
}

}  // namespace mle::ui
