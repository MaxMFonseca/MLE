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
    auto* ui_image = ui_.render();
    return ui_image;
};
}  // namespace mle::user
