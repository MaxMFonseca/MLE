#include "UITest.h"

#include "ModelTest.h"
#include "mle/client/Client.h"
#include "mle/renderer/Renderer.h"

namespace mle::user {
void UITestLayer::init() {
    MLE_I("UITest::init()");

    Client::i().getGameLayerTable()["ui"] = &ui_;

    ui_.setRoot("i/ui/ui_tests/Layer");
};

void UITestLayer::update() {
    ui_.update();
};

ImageRef UITestLayer::render() {
    auto* image = render_target_.getImage();

    auto* ui_image = ui_.render();

    if (ui_image) {
        image->blend(Renderer::i().frameRenderer().cmd(), *ui_image);
    }

    return image;
};

void UITestLayer::shutdown() {
    ui_.shutdown();
};
}  // namespace mle::user
