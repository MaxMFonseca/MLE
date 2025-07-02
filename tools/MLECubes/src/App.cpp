#include "App.h"

#include "mle/lua/Lua.h"
#include "mle/ui/UI.h"

namespace mle_cubes::app {
namespace {
class Impl {
  public:
    inline void init();
    inline void shutdown();

    inline void update();
    inline void render();

  private:
    std::unique_ptr<Editor> editor_;
};

void Impl::init() {  // NOLINT
    MLE_C("MLECubes App initializing");

    mle::ui::setNextRoot(mle::lua::require("i/ui/init/layout"));
}

void Impl::shutdown() {  // NOLINT
    MLE_C("MLECubes App shutting down");
}

void Impl::update() {  // NOLINT
    static int update_count = 0;
    if (update_count < 35) {
        update_count++;
        if (update_count == 35) {
            mle::ui::setNextRoot(mle::lua::require("i/ui/editor/layout"));
            editor_ = std::make_unique<Editor>();
            editor_->init();
        }
    }
}

void Impl::render() {  // NOLINT
}

std::unique_ptr<Impl> i_;  // NOLINT
}  // namespace

void init() {
    MLE_ASSERT(!i_);
    i_ = std::make_unique<Impl>();
    i_->init();
}

void shutdown() {
    MLE_ASSERT(i_);
    i_->shutdown();
}

void update() {
    MLE_ASSERT(i_);
    i_->update();
}

void render() {
    MLE_ASSERT(i_);
    i_->render();
}
}  // namespace mle_cubes::app
