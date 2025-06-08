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
#include "mle/ui/detail/ElementManager.h"

namespace mle::ui {
namespace {
class Impl {
  public:
    void init();
    void shutdown();
    renderer::ImageRef render();

    auto& getElementManager() { return element_manager_; }

  private:
    detail::ElementManager element_manager_;
};

struct Position {
    vec2f pos;
};

void Impl::init() {
    element_manager_.init();

    element_manager_.resetRoot(lua::require("testui", true));
}

void Impl::shutdown() {
    element_manager_.shutdown();
}

renderer::ImageRef Impl::render() {
    return element_manager_.render();
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

renderer::ImageRef render() {
    MLE_ASSERT(i_);
    return i_->render();
}

namespace detail {
ElementManager& getElementManager() {
    MLE_ASSERT(i_);
    return i_->getElementManager();
}
}  // namespace detail
}  // namespace mle::ui
