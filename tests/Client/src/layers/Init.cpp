#include "Init.h"

namespace mle::user {
void InitLayer::init() {
    MLE_I("InitLayer::init()");

    ui_.setRoot("i/ui/Test");
};

void InitLayer::update() {
    ui_.update();
};

ImageRef InitLayer::render([[maybe_unused]] f64 dt) {
    ui_.render();
    return nullptr;
};
}  // namespace mle::user
