#include "UI.h"

#include <memory>

#include "mle/common/Assert.h"
#include "mle/lua/Lua.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"
#include "mle/ui/element/Base.h"
#include "mle/ui/element/Container.h"
#include "mle/ui/element/LuaKeyHandlers.h"

namespace mle::ui {
namespace {
class Impl {
  public:
    inline void init();
    inline void shutdown();

    inline void update();
    inline renderer::ImageRef render();

    auto& getRegistry() { return registry_; }

  private:
    void checkElementsBoundChanged();

  private:
    entt::registry registry_;
    entt::entity root_ = entt::null;
};

struct Position {
    vec2f pos;
};

void Impl::init() {
    element::addEngineLuaKeyHandlers();

    root_ = registry_.create();
    element::applyTable(root_, lua::require("testui", true));

    MLE_ASSERT_LOG(registry_.try_get<element::comp::RootImage>(root_), "The root element must have the root_image field set.");
}

void Impl::shutdown() {
}

void Impl::update() {
    // solveUserInput();  // !! Compute colision on render data

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
        registry_.get<element::comp::Container>(e).updateChildrenBounds(e);
    } else {
        registry_.get<element::comp::Container>(root_).updateChildrenBounds(root_, {}, true);
    }

    registry_.clear<element::comp::ChildChangedBounds>();
}

renderer::ImageRef Impl::render() {
    registry_.get<element::comp::Renderable>(root_).render(root_, nullptr);

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

}  // namespace mle::ui
