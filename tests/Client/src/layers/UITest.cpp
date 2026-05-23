#include "UITest.h"

#include "ModelTest.h"
#include "mle/client/Client.h"
#include "mle/renderer/Renderer.h"
#include "mle/window/Window.h"

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
    auto* image = getImage();

    auto* ui_image = ui_.render();

    if (ui_image) {
        image->blend(Renderer::i().frameRenderer().cmd(), *ui_image);
    }

    return image;
};

void UITestLayer::shutdown() {
    ui_.shutdown();
};

ImageRef UITestLayer::getImage() {
    auto& frame_renderer = Renderer::i().frameRenderer();
    auto frame_idx = frame_renderer.getCurrentFrameId();
    auto& image = images_.at(frame_idx);
    vec2u size = Window::i().getSize();

    if (!image) {
        Image::CI image_ci{};
        image_ci.extent = size;
        image_ci.format = Image::Format::COLOR;
        image_ci.extra_usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
        image = Image::createHnd(image_ci);
    } else if (image->getExtent() != size) {
        frame_renderer.deleteAfterFrame(std::move(image));
        Image::CI image_ci{};
        image_ci.extent = size;
        image_ci.format = Image::Format::COLOR;
        image_ci.extra_usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
        image = Image::createHnd(image_ci);
    }

    image->clear(Renderer::i().frameRenderer().cmd());

    return image.get();
};
}  // namespace mle::user
