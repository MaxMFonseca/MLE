#include "TextureAtlas.h"

#include "mle/renderer/Buffer.h"
#include "mle/renderer/Renderer.h"
#include "mle/utils/ECS.h"

namespace mle {
void TextureAtlas::init(const CreateInfo& ci) {
    Image::CI image_ci;
    image_ci.extent = ci.extent;
    image_ci.format = ci.format;
    image_ = Image::createHnd(image_ci);
    packer_ = RectPacker(ci.extent, ci.padding);
}

// NOLINTNEXTLINE(misc-no-recursion) Know
[[nodiscard]] Expected<std::pair<ImageRef, TextureAtlas::Entry>> TextureAtlas::get(entt::id_type id) const {
    auto it = entries_.find(id);
    if (it == entries_.end()) {
        if (next_atlas_) {
            return next_atlas_->get(id);
        }

        MLE_E("TextureAtlas::get: Entry not found for id {}", id);
        return std::unexpected(Result::NOT_FOUND);
    }
    return std::make_pair(image_.get(), it->second);
}

void TextureAtlas::enqueueCopy(entt::id_type id, const Image::RawData& data) {
    if (data.extent.x > image_->getExtent().x || data.extent.y > image_->getExtent().y) {
        MLE_E("TextureAtlas::add: Data extent larger than atlas for id {}: data extent = ({}, {}), atlas extent = ({}, {})", id, data.extent.x, data.extent.y,
              image_->getExtent().x, image_->getExtent().y);
        return;
    }

    if (data.channels != image_->getChannelCount()) {
        MLE_E("TextureAtlas::add: Channel count mismatch for id {}: data channels = {}, atlas channels = {}", id, data.channels, image_->getChannelCount());
        return;
    }

    Buffer::CI buffer_ci;
    buffer_ci.size = data.pixels.size();
    buffer_ci.usage = vk::BufferUsageFlagBits::eTransferSrc;
    buffer_ci.allocation_type = Buffer::CI::AllocationType::STAGING;
    auto buffer = Buffer::createHnd(buffer_ci);
    buffer->write(data.pixels.data(), data.pixels.size());
    pending_data_.emplace_back(id, data.extent, std::move(buffer));
}

void TextureAtlas::enqueueCopy(entt::id_type id, vec2u vec, BufferHnd&& buffer) {
    if (vec.x > image_->getExtent().x || vec.y > image_->getExtent().y) {
        MLE_E("TextureAtlas::add: Data extent larger than atlas for id {}: data extent = ({}, {}), atlas extent = ({}, {})", id, vec.x, vec.y,
              image_->getExtent().x, image_->getExtent().y);
        return;
    }

    pending_data_.emplace_back(id, vec, std::move(buffer));
}

Expected<std::pair<ImageRef, TextureAtlas::Entry>> TextureAtlas::copyOnFrame(entt::id_type id, const Image::RawData& data) {
    if (data.extent.x > image_->getExtent().x || data.extent.y > image_->getExtent().y) {
        MLE_E("TextureAtlas::copyOnFrame: Data extent larger than atlas for id {}: data extent = ({}, {}), atlas extent = ({}, {})", id, data.extent.x,
              data.extent.y, image_->getExtent().x, image_->getExtent().y);
        return std::unexpected(Result::INVALID_ARGUMENT);
    }

    if (data.channels != image_->getChannelCount()) {
        MLE_E("TextureAtlas::copyOnFrame: Channel count mismatch for id {}: data channels = {}, atlas channels = {}", id, data.channels,
              image_->getChannelCount());
        return std::unexpected(Result::INVALID_ARGUMENT);
    }

    Buffer::CI buffer_ci;
    buffer_ci.size = data.pixels.size();
    buffer_ci.usage = vk::BufferUsageFlagBits::eTransferSrc;
    buffer_ci.allocation_type = Buffer::CI::AllocationType::STAGING;
    auto buffer = Buffer::createHnd(buffer_ci);
    buffer->write(data.pixels.data(), data.pixels.size());
    pending_data_.emplace_back(id, data.extent, std::move(buffer));

    updateOnFrame();

    return get(id);
}

// NOLINTNEXTLINE(misc-no-recursion) Know
Expected<std::pair<ImageRef, TextureAtlas::Entry>> TextureAtlas::copyOnFrame(entt::id_type id, ImageRef image) {
    if (image->getExtent().x > image_->getExtent().x || image->getExtent().y > image_->getExtent().y) {
        MLE_E("TextureAtlas::copyOnFrame: Image extent larger than atlas for id {}: image extent = ({}, {}), atlas extent = ({}, {})", id, image->getExtent().x,
              image->getExtent().y, image_->getExtent().x, image_->getExtent().y);
        return std::unexpected(Result::INVALID_ARGUMENT);
    }

    auto packed_r = packer_.tryPackOne(image->getExtent());
    if (!packed_r) {
        if (!next_atlas_) {
            createChildAtlas();
        }
        return next_atlas_->copyOnFrame(id, image);
    }
    vec2u pos = *packed_r;

    image_->copyImage(Renderer::i().frameRenderer().cmd(), *image, image->getExtent(), {0, 0}, pos);

    Rectf region;
    region.setPos(pos);
    region.setSize(image->getExtent());

    vec2f image_extent_f = vec2f{image_->getExtent()};

    Entry entry;
    entry.setPos(region.pos() / image_extent_f);
    entry.setSize(region.size() / image_extent_f);
    entries_.emplace(id, entry);

    image_->transitionState(Renderer::i().frameRenderer().cmd(), Image::State::FS_READ);

    return std::make_pair(image_.get(), entry);
}

// NOLINTNEXTLINE(misc-no-recursion) Know
void TextureAtlas::updateOnFrame() {
    if (pending_data_.empty()) {
        return;
    }

    auto pd_it = pending_data_.begin();
    while (pd_it != pending_data_.end()) {
        const vec2u size = std::get<vec2u>(*pd_it);
        auto packed_r = packer_.tryPackOne(size);
        if (!packed_r) {
            break;
        }

        vec2u pos = *packed_r;

        Recti region;
        region.setPos(pos);
        region.setSize(size);

        image_->copyBuffer(Renderer::i().frameRenderer().cmd(), *std::get<BufferHnd>(*pd_it), region);

        vec2f image_extent_f = vec2f{image_->getExtent()};

        Entry entry;
        entry.setPos(vec2f(region.pos()) / image_extent_f);
        entry.setSize(vec2f(region.size()) / image_extent_f);
        entries_.emplace(std::get<entt::id_type>(*pd_it), entry);
        Renderer::i().frameRenderer().deleteAfterFrame(std::move(std::get<BufferHnd>(*pd_it)));
        pd_it++;
    }

    pending_data_.erase(pending_data_.begin(), pd_it);

    image_->transitionState(Renderer::i().frameRenderer().cmd(), Image::State::FS_READ);

    if (!pending_data_.empty()) {
        if (!next_atlas_) {
            createChildAtlas();
        }

        for (auto& pd : pending_data_) {
            next_atlas_->enqueueCopy(std::get<entt::id_type>(pd), std::get<vec2u>(pd), std::move(std::get<BufferHnd>(pd)));
        }
        pending_data_.clear();
        next_atlas_->updateOnFrame();
    }
}

void TextureAtlas::createChildAtlas() {
    next_atlas_ = std::make_unique<TextureAtlas>();
    CreateInfo ci;
    ci.extent = image_->getExtent();
    ci.format = image_->getFormat();
    next_atlas_->init(ci);
}
}  // namespace mle
