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
#include "mle/ui/Events.h"
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

    auto& getED() { return ed_; }

    void namedElementCreated(const std::string& name, entt::entity element);
    void listenNamedElementCreated(const std::string& name, std::function<void(entt::entity)>&& callback);

    ID addUIEventListner(const std::string& name, std::move_only_function<void(const sol::object& o)>&& fn, bool once = false);
    void enqueueUIEvent(const std::string& name, const sol::object& o) { events_.emplace_back(name, o); }
    void removeUIEventListener(const std::string& name, ID id) { remove_listeners_.emplace_back(name, id); }

  private:
    void checkElementsBoundChanged();

    void nextRoot();

  private:
    ED ed_;

    entt::registry registry_;
    entt::entity root_ = entt::null;
    detail::FontCache font_cache_;

    vk::CommandBuffer pre_render_cmd_buffer_;
    std::vector<vk::CommandBuffer> s_command_buffers_;

    vec2u root_size_{0};

    // TODO: make this a function instead so we can perform some sort of media query
    sol::table next_root_{};

    std::map<std::string, std::vector<std::function<void(entt::entity)>>> named_element_created_listeners_;

    std::vector<std::pair<std::string, const sol::object>> events_;
    std::vector<std::pair<std::string, ID>> remove_listeners_;
    struct Listener {
        ID id;
        std::move_only_function<void(const sol::object&)> fn;
        bool once = false;
    };
    std::map<std::string, std::vector<Listener>> ui_listeners_;
};

ID Impl::addUIEventListner(const std::string& name, std::move_only_function<void(const sol::object& o)>&& fn, bool once) {
    ID id = genID();
    ui_listeners_[name].emplace_back(Listener{.id = id, .fn = std::move(fn), .once = once});
    return id;
}

void Impl::listenNamedElementCreated(const std::string& name, std::function<void(entt::entity)>&& callback) {
    named_element_created_listeners_[name].emplace_back(std::move(callback));
}

void Impl::namedElementCreated(const std::string& name, entt::entity element) {
    auto it = named_element_created_listeners_.find(name);
    if (it == named_element_created_listeners_.end()) {
        return;
    }

    for (const auto& fn : it->second) {
        fn(element);
    }

    named_element_created_listeners_.erase(it);
}

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
    ui_table["enqueue"] = [this](const std::string& name, const sol::object& obj) { enqueueUIEvent(name, obj); };

    lua::getMleTable()["ui"] = ui_table;
}

void Impl::shutdown() {
    MLE_I("Shutting down UI");

    font_cache_.reset();
}

void Impl::update() {
    if (root_ != entt::null) {
        vec2u cursor = window::getUIM().getCursorPos();
        auto& root_container = registry_.get<element::comp::Container>(root_);
        root_container.collide(root_, cursor);

        auto view = registry_.view<element::comp::Collidable>();
        for (auto e : view) {
            view.get<element::comp::Collidable>(e).update(e);
        }

        for (const auto& e : events_) {
            auto listeners_it = ui_listeners_.find(e.first);
            if (listeners_it != ui_listeners_.end()) {
                for (auto& [id, fn, once] : listeners_it->second) {
                    fn(e.second);
                    if (once) {
                        remove_listeners_.emplace_back(e.first, id);
                    }
                }
            }
        }
        events_.clear();

        for (auto& [name, id] : remove_listeners_) {
            auto listeners_it = ui_listeners_.find(name);
            MLE_ASSERT_LOG(listeners_it != ui_listeners_.end(), "Trying to remove listener for event '{}' but no listeners were found.", name);
            auto listener_it = std::ranges::find_if(listeners_it->second, [id](const auto& l) { return l.id == id; });
            MLE_ASSERT_LOG(listener_it != listeners_it->second.end(), "Trying to remove listener with id '{}' for event '{}' but no such listener was found.",
                           id, name);
            listeners_it->second.erase(listener_it);
        }
        remove_listeners_.clear();
    }

    if (next_root_) {
        nextRoot();
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

    ed_.dispatch(events::RootCreated{});
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

void namedElementCreated(const std::string& name, entt::entity element) {
    MLE_ASSERT(i_);
    i_->namedElementCreated(name, element);
}

void listenNamedElementCreated(const std::string& name, std::function<void(entt::entity)>&& callback) {
    MLE_ASSERT(i_);
    i_->listenNamedElementCreated(name, std::move(callback));
}

ID listenUIEvent(const std::string& name, std::move_only_function<void(const sol::object& o)>&& func, bool once) {
    MLE_ASSERT(i_);
    return i_->addUIEventListner(name, std::move(func), once);
};

void removeUIEventListener(const std::string& name, ID id) {
    MLE_ASSERT(i_);
    i_->removeUIEventListener(name, id);
};

void dispatchUIEvent(const std::string& name, const sol::object& o) {
    MLE_ASSERT(i_);
    i_->enqueueUIEvent(name, o);
};

namespace detail {
ED& getED() {
    MLE_ASSERT(i_);
    return i_->getED();
}
}  // namespace detail

}  // namespace mle::ui
