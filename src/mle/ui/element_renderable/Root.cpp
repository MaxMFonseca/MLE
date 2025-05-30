#include "Root.h"

#include "mle/renderer/Pipeline.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/element_renderable/FlexContainer.h"
#include "mle/ui/element_renderable/Shader.h"

namespace mle::ui::element_renderable {
namespace {
void renderRects(std::vector<RectInstance>& rects) {
    if (rects.empty()) {
        return;
    }

    renderer::Buffer::CI buffer_ci;
    buffer_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    buffer_ci.allocation_type = renderer::Buffer::CI::AllocationType::GPU_ONLY_HOST_WRITE_SEQ;
    buffer_ci.size = sizeof(RectInstance) * rects.size();
    renderer::BufferRef rect_instance_buffer = renderer::createBufferForFrame(buffer_ci);
    rect_instance_buffer->update(rects.data());

    renderer::Pipeline::ExecInfo exec_info;
    exec_info.instance_buffer = rect_instance_buffer;
    exec_info.instance_count = rects.size();
    exec_info.index_count = 4;

    ui::getRectPipeline()->exec(exec_info);
}

void renderSprites(std::vector<SpriteInstance>& sprites) {
    if (sprites.empty()) {
        return;
    }

    renderer::Buffer::CI buffer_ci;
    buffer_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    buffer_ci.allocation_type = renderer::Buffer::CI::AllocationType::GPU_ONLY_HOST_WRITE_SEQ;
    buffer_ci.size = sizeof(SpriteInstance) * sprites.size();
    renderer::BufferRef sprite_instance_buffer = renderer::createBufferForFrame(buffer_ci);
    sprite_instance_buffer->update(sprites.data());

    renderer::Pipeline::ExecInfo exec_info;
    exec_info.instance_buffer = sprite_instance_buffer;
    exec_info.instance_count = sprites.size();
    exec_info.index_count = 4;

    auto& d0 = exec_info.descriptor_sets.emplace_back();
    d0 = ui::getSpritePipelineD0Write();

    getSpritePipeline()->exec(exec_info);
}

void renderText(std::vector<TextInstance>& texts) {
    if (texts.empty()) {
        return;
    }

    renderer::Buffer::CI buffer_ci;
    buffer_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    buffer_ci.allocation_type = renderer::Buffer::CI::AllocationType::GPU_ONLY_HOST_WRITE_SEQ;
    buffer_ci.size = sizeof(TextInstance) * texts.size();
    renderer::BufferRef text_instance_buffer = renderer::createBufferForFrame(buffer_ci);
    text_instance_buffer->update(texts.data());

    renderer::Pipeline::ExecInfo exec_info;
    exec_info.instance_buffer = text_instance_buffer;
    exec_info.instance_count = texts.size();
    exec_info.index_count = 4;

    auto& d0 = exec_info.descriptor_sets.emplace_back();
    d0 = ui::getTextPipelineD0Write();

    ui::getTextPipeline()->exec(exec_info);
}

void renderImages(std::vector<std::pair<ImageInstance, renderer::ImageRef>>& images) {
    if (images.empty()) {
        return;
    }

    renderer::Buffer::CI buffer_ci;
    buffer_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    buffer_ci.allocation_type = renderer::Buffer::CI::AllocationType::GPU_ONLY_HOST_WRITE_SEQ;
    buffer_ci.size = sizeof(ImageInstance) * images.size();
    renderer::BufferRef image_instance_buffer = renderer::createBufferForFrame(buffer_ci);

    for (usize i = 0; i < images.size(); ++i) {
        image_instance_buffer->update(&images[i].first, sizeof(ImageInstance), i * sizeof(ImageInstance));
        renderer::Pipeline::ExecInfo exec_info;
        exec_info.instance_buffer = image_instance_buffer;
        exec_info.instance_buffer_offset = i * sizeof(ImageInstance);
        exec_info.instance_count = 1;
        exec_info.index_count = 4;

        vk::DescriptorImageInfo image_info;
        image_info.imageLayout = vk::ImageLayout::eReadOnlyOptimal;
        image_info.imageView = images[i].second->getDefaultView();
        image_info.sampler = ui::getNearestSampler();

        auto& d0 = exec_info.descriptor_sets.emplace_back();
        d0.dstBinding = 0;
        d0.descriptorCount = 1;
        d0.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        d0.setImageInfo(image_info);

        ui::getImagePipeline()->exec(exec_info);
    }
}

void renderShaders(std::vector<ShaderConstRef>& shaders) {
    for (auto& shader : shaders) {
        shader->exec();
    }
}
}  // namespace

Root::Root(ElementRef element) :
    FlexContainer(element) {
    MLE_LOG_THIS;
    element_->setRoot();
}

void Root::set(const sol::table& table) {
    if (sol::object r_cc = table["clear_color"]; r_cc.valid()) {
        setClearColor(r_cc);
    }
    FlexContainer::set(table);
}

void Root::createDefaultImage() {
    renderer::Image::CI ci;
    ci.format = format_ = renderer::getDefaultColorFormat();
    ci.extent = element_->getSizeReal();
    ci.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;

    if (image_) {
        renderer::addToFrameDeleteQueue(std::move(image_));
    }
    image_ = std::make_unique<renderer::Image>(ci);
}

void Root::createImages() {
    createDefaultImage();
    element_->setRequestingRender();
}

bool Root::render() {
    if (images_dirty_) {
        MLE_T("Element {} has dirty images", element_->getName());
        createImages();
        images_dirty_ = false;
        need_render_internal_ = true;
    }

    need_render_internal_ |= element_->isRequestingRender();
    need_render_internal_ |= Container::render();

    if (need_render_internal_) {
        image_->transitionStateInFrame(renderer::Image::State::COLOR_ATT);

        RenderableData renderable_data;
        if (image_->getFormat() == renderer::getDefaultColorFormat()) {
            for (auto& child : children_) {
                child.second->getRenderData(0, renderable_data);
            }
        } else {
            MLE_W("Image format is not swapchain format, cannot use default ui pipelines");
        }

        renderer::RenderingInfo render_info;
        auto& c0 = render_info.colors.emplace_back();
        c0.image = image_.get();
        c0.load = vk::AttachmentLoadOp::eClear;
        c0.clear_value = renderer::toVkClearColor(clear_color_);
        renderer::beginRendering(render_info);

        for (auto& layer_data : renderable_data) {
            renderRects(layer_data.second.rects);
            renderSprites(layer_data.second.sprites);
            renderText(layer_data.second.texts);
            renderImages(layer_data.second.images);
            renderShaders(layer_data.second.shaders);
        }

        postProc();

        renderer::endRendering();

        need_render_internal_ = false;
        return true;
    }

    return false;
}

void Root::getRenderData(int layer, RenderableData& data) const {
    ImageInstance ii{};
    auto r = element_->getRectOnRootClamped();
    ii.pos = r.pos;
    ii.size = r.size;
    ii.texture_offset = texture_rect_clamped_.pos;
    ii.texture_size = texture_rect_clamped_.size;
    ii.color = tint_.withAMultiplied(element_->getOpacity());

    image_->transitionStateInFrame(renderer::Image::State::SHADER_READ);
    data[layer].images.emplace_back(ii, image_.get());
}

void Root::updateInternalBounds(std::optional<Element::NewBounds> new_bounds) {
    FlexContainer::updateInternalBounds(new_bounds);

    if (!new_bounds) {
        return;
    }

    Rectf relative_transform = calculateRelativeTransformation(element_->getRectOnRoot(), element_->getRectOnRootClamped());

    texture_rect_clamped_.pos = relative_transform.pos;
    texture_rect_clamped_.size = relative_transform.size;

    visible_ = !(texture_rect_clamped_.size.x <= 0.0F || texture_rect_clamped_.size.y <= 0.0);

    element_->setRequestingRender();
}

Root& Root::get(ElementRef e) {
    if constexpr (IS_DEBUG_BUILD) {
        auto* ptr = dynamic_cast<Root*>(e->getRenderable());
        MLE_ASSERT(ptr);
        return *ptr;
    } else {
        return *static_cast<Root*>(e->getRenderable());
    }
}

void Root::setFormat(vk::Format format) {
    format_ = format;
    images_dirty_ = true;
}

void Root::log(const std::string& prefix) const {
    MLE_T("{}  Root: {}", prefix, (void*)this);
    Container::log(prefix);
}
}  // namespace mle::ui::element_renderable
