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
}

void Impl::shutdown() {
}

void Impl::update() {
}

renderer::ImageRef Impl::render() {
    return nullptr;
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
