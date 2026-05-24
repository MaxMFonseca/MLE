#include "Init.h"

#include "ModelTest.h"
#include "UITest.h"
#include "mle/client/Client.h"
#include "mle/renderer/Renderer.h"

namespace mle::user {
void InitLayer::init() {
    MLE_I("InitLayer::init()");

    Client::i().getGameLayerTable()["init_model_test"] = []() { Client::i().pushGameLayer(std::make_unique<ModelTestLayer>()); };
    Client::i().getGameLayerTable()["init_ui_test"] = []() { Client::i().pushGameLayer(std::make_unique<UITestLayer>()); };

    ui_.setRoot("i/ui/InitLayer");
};

void InitLayer::update() {
    ui_.update();
};

ImageRef InitLayer::render() {
    auto* image = render_target_.getImage();

    auto* ui_image = ui_.render();

    if (ui_image) {
        image->blend(Renderer::i().frameRenderer().cmd(), *ui_image);
    }

    return image;
};

void InitLayer::shutdown() {
    ui_.shutdown();
};
}  // namespace mle::user
