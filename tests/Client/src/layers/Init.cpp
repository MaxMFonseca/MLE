#include "Init.h"

namespace mle::user {
void InitLayer::init() {
    MLE_I("InitLayer::init()");

    ui_.setRoot("i/ui/Test");
};

void InitLayer::update() {
    ui_.update();
};
}  // namespace mle::user
