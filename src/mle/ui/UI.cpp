#include "UI.h"

#include <memory>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "mle/common/Assert.h"
#include "mle/common/Logger.h"
#include "mle/common/RectPacker.h"
#include "mle/common/Utils.h"
#include "mle/core/Core.h"
#include "mle/lua/Lua.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"
#include "mle/ui/Types.h"
#include "mle/ui/element/Base.h"
#include "mle/ui/element/Collidable.h"
#include "mle/ui/element/Container.h"
#include "mle/ui/element/LuaKeyHandlers.h"
#include "mle/ui/element/Types.h"
#include "mle/window/Window.h"
#include "utf8/unchecked.h"

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
    void setNextRoot(const sol::table& next_root) { next_root_ = next_root; }

    ID addListener(const std::string& event_name, std::function<void()>&& callback);
    void removeListener(const std::string& event_name, ID id);
    void dispatchEvent(const std::string& event_name);

  private:
    void checkElementsBoundChanged();

    void nextRoot();

  private:
    entt::registry registry_;
    entt::entity root_ = entt::null;
    detail::FontCache font_cache_;

    vk::CommandBuffer pre_render_cmd_buffer_;
    std::vector<vk::CommandBuffer> s_command_buffers_;

    vec2u root_size_{0};

    // TODO: make this a function insead so we can perform some sort of media query
    sol::table next_root_{};

    // TODO: improve tihs
    std::map<std::string, std::map<ID, std::pair<std::function<void(void)>, bool>>> event_listeners_;
};

struct Position {
    vec2f pos;
};

void Impl::init() {
    MLE_I("Initializing UI");

    root_size_ = window::getSize();

    element::addEngineLuaKeyHandlers();

    font_cache_.init();

    Font::CI default_font_ci;
    default_font_ci.path = "mle/DigitalDisco.ttf";
    default_font_ci.pre_load_string = U"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;':\",.<>?/~`";
    font_cache_.add(std::move(default_font_ci));

    auto ui_table = lua::createTable();
    ui_table["table"] = lua::createTable();

    lua::getMleTable()["ui"] = ui_table;
}

void Impl::shutdown() {
    MLE_I("Shutting down UI");

    font_cache_.reset();
}

void Impl::update() {
    for (auto e : event_listeners_) {
        for (auto it = e.second.begin(); it != e.second.end();) {
            if (!it->second.second) {
                it = e.second.erase(it);
            } else {
                ++it;
            }
        }
    }

    if (next_root_) {
        nextRoot();
    } else {
        vec2u cursor = window::getUIM().getCursorPos();
        auto& root_container = registry_.get<element::comp::Container>(root_);
        root_container.collide(root_, cursor);

        auto view = registry_.view<element::comp::Collidable>();
        for (auto e : view) {
            view.get<element::comp::Collidable>(e).update(e);
        }
    }

    auto on_init_view = registry_.view<element::comp::OnInit>();
    for (auto e : on_init_view) {
        on_init_view.get<element::comp::OnInit>(e).fn(e);
    }
    registry_.clear<element::comp::OnInit>();

    auto on_update_view = registry_.view<element::comp::OnUpdate>();
    for (auto e : on_update_view) {
        on_update_view.get<element::comp::OnUpdate>(e).fn(e);
    }

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

void Impl::nextRoot() {
    MLE_I("Switching root element.");

    renderer::detail::waitIdle();

    registry_.clear();
    root_ = registry_.create();
    element::applyEntityTable(root_, next_root_);
    next_root_.reset();

    MLE_ASSERT_LOG(registry_.try_get<element::comp::RootImage>(root_), "The root element must have the root_image field set.");

    MLE_I("New root element created.");
}

renderer::ImageRef Impl::render() {
    renderer::RenderingThread pre_render_thread;
    pre_render_thread.init();
    pre_render_cmd_buffer_ = pre_render_thread.cmd();

    registry_.get<element::comp::Renderable>(root_).render({.self = root_});

    pre_render_thread.submit();
    for (auto cmd : s_command_buffers_) {
        renderer::submitJobOnFrame(cmd);
    }
    s_command_buffers_.clear();

    renderer::ImageRef root_image = registry_.try_get<element::comp::RootImage>(root_)->image_handle.get();

    return root_image;
}

ID Impl::addListener(const std::string& event_name, std::function<void()>&& callback) {
    auto& listeners = event_listeners_[event_name];
    ID id = genID();
    listeners[id].first = std::move(callback);
    listeners[id].second = false;
    MLE_T("Adding UI event listener for {} with ID {}", event_name, id);
    return id;
}

void Impl::removeListener(const std::string& event_name, ID id) {
    MLE_T("Removing UI event listener for {} with ID {}", event_name, id);
    auto event_type_it = event_listeners_.find(event_name);
    if (event_type_it != event_listeners_.end()) {
        auto listener_it = event_type_it->second.find(id);
        if (listener_it != event_type_it->second.end()) {
            listener_it->second.second = true;
        }
    }
}

void Impl::dispatchEvent(const std::string& event_name) {
    MLE_T("Dispatching UI event {}", event_name);
    auto it = event_listeners_.find(event_name);
    if (it != event_listeners_.end()) {
        for (const auto& [id, callback] : it->second) {
            if (!callback.second) {
                callback.first();
            }
        }
    }
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

void setNextRoot(const sol::table& next_root) {
    MLE_ASSERT(i_);
    i_->setNextRoot(next_root);
}

ID addListener(const std::string& event_name, std::function<void()>&& callback) {
    MLE_ASSERT(i_);
    return i_->addListener(event_name, std::move(callback));
}

void removeListener(const std::string& event_name, ID id) {
    MLE_ASSERT(i_);
    i_->removeListener(event_name, id);
}

void dispatchEvent(const std::string& event_name) {
    MLE_ASSERT(i_);
    i_->dispatchEvent(event_name);
}

entt::entity getElement(const std::string& name) {
    MLE_ASSERT(i_);
    auto view = i_->getRegistry().view<element::comp::Name>();
    for (auto e : view) {
        if (view.get<element::comp::Name>(e).name == name) {
            return e;
        }
    }
    MLE_E("Element with name '{}' not found", name);
    return entt::null;
}
}  // namespace mle::ui
