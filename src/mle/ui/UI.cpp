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

  private:
    detail::ElementManager element_manager_;

    renderer::ImageHnd image_;
    renderer::PipelineHnd pipeline_;
};

struct Position {
    vec2f pos;
};

void Impl::init() {
    renderer::Image::CI image_ci;
    image_ci.extent = {800, 600};
    image_ci.format = renderer::getDefaultColorFormat();
    image_ci.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
    image_ = renderer::Image::createHnd(image_ci);

    renderer::Pipeline::CI pipeline_ci;
    pipeline_ci.vertex_shader = renderer::getShader("triangle.vert", true);
    pipeline_ci.fragment_shader = renderer::getShader("triangle.frag", true);
    pipeline_ci.color_attachment_formats.push_back(renderer::getDefaultColorFormat());
    pipeline_ci.blend_attachments = renderer::makeDefaultBlendAttachmentStates(1);
    pipeline_ = renderer::Pipeline::createHnd(pipeline_ci);

    element_manager_.reset(lua::require("testui", true));
}

void Impl::shutdown() {
    element_manager_.reset();
}

renderer::ImageRef Impl::render() {
    renderer::RenderingThread thread;
    thread.init(true);

    renderer::AttachmentInfo color_attachment;
    color_attachment.image = image_.get();
    color_attachment.load = vk::AttachmentLoadOp::eClear;
    color_attachment.store = vk::AttachmentStoreOp::eStore;
    color_attachment.clear_value.color = std::array<float, 4>{0.2F, 0.2F, 0.2F, 1.0F};
    renderer::RenderingInfo rendering_info;
    rendering_info.pipeline = pipeline_.get();
    rendering_info.colors.push_back(color_attachment);
    thread.beginRendering(rendering_info);
    thread.setViewport();
    thread.draw(1, 3);
    thread.end();

    element_manager_.render();

    return image_.get();
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

}  // namespace mle::ui
