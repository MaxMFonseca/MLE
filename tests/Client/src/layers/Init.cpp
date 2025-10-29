#include "Init.h"

#include "mle/renderer/Renderer.h"
#include "mle/window/Window.h"

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

    auto* image = getImage();

    if (ui_image) {
        image->blend(Renderer::i().frameRenderer().cmd(), *ui_image);
    }

    return image;
};

void InitLayer::shutdown() {
    ui_.shutdown();
};

ImageRef InitLayer::getImage() {
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

    return image.get();
};
}  // namespace mle::user
