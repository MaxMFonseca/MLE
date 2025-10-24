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

TextureAtlas& TextureAtlas::getOrCreateNextAtlas() {
    if (!next_atlas_) {
        next_atlas_ = std::make_unique<TextureAtlas>();
        CreateInfo ci;
        ci.extent = image_->getExtent();
        ci.format = image_->getFormat();
        next_atlas_->init(ci);
    }
    return *next_atlas_;
}

void TextureAtlas::emplaceEntry(entt::id_type id, const Recti& region) {
    vec2f image_extent_f = vec2f{image_->getExtent()};

    Entry entry;
    entry.setPos(vec2f(region.pos()) / image_extent_f);
    entry.setSize(vec2f(region.size()) / image_extent_f);
    entries_.emplace(id, entry);
}

// NOLINTNEXTLINE(misc-no-recursion) Know
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

    auto packer_r = packer_.tryPackOne(data.extent);

    if (!packer_r) {
        getOrCreateNextAtlas().enqueueCopy(id, data);
        return;
    }

    Buffer::CI buffer_ci;
    buffer_ci.size = data.pixels.size();
    buffer_ci.usage = vk::BufferUsageFlagBits::eTransferSrc;
    buffer_ci.allocation_type = Buffer::CI::AllocationType::STAGING;
    auto buffer = Buffer::createHnd(buffer_ci);
    buffer->write(data.pixels.data(), data.pixels.size());
    pending_data_.emplace_back(id, Recti{packer_r.value(), data.extent}, std::move(buffer));
    emplaceEntry(id, {packer_r.value(), data.extent});
}

// NOLINTNEXTLINE(misc-no-recursion) Know
void TextureAtlas::enqueueCopy(entt::id_type id, vec2u size, BufferHnd&& buffer) {
    if (size.x > image_->getExtent().x || size.y > image_->getExtent().y) {
        MLE_E("TextureAtlas::add: Data extent larger than atlas for id {}: data extent = ({}, {}), atlas extent = ({}, {})", id, size.x, size.y,
              image_->getExtent().x, image_->getExtent().y);
        return;
    }

    auto packer_r = packer_.tryPackOne(size);

    if (!packer_r) {
        getOrCreateNextAtlas().enqueueCopy(id, size, std::move(buffer));
        return;
    }

    pending_data_.emplace_back(id, Recti{packer_r.value(), size}, std::move(buffer));
    emplaceEntry(id, {packer_r.value(), size});
}

// NOLINTNEXTLINE(misc-no-recursion) Know
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

    auto packer_r = packer_.tryPackOne(data.extent);
    if (!packer_r) {
        return getOrCreateNextAtlas().copyOnFrame(id, data);
    }
    vec2u pos = *packer_r;

    Buffer::CI buffer_ci;
    buffer_ci.size = data.pixels.size();
    buffer_ci.usage = vk::BufferUsageFlagBits::eTransferSrc;
    buffer_ci.allocation_type = Buffer::CI::AllocationType::STAGING;
    auto buffer = Buffer::createHnd(buffer_ci);
    buffer->write(data.pixels.data(), data.pixels.size());
    pending_data_.emplace_back(id, Recti{pos, data.extent}, std::move(buffer));

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

    if (image->getFormat() != image_->getFormat()) {
        MLE_E("TextureAtlas::copyOnFrame: format mismatch for id {}: image format = {}, atlas format = {}", id, image->getFormat(), image_->getFormat());
        return std::unexpected(Result::INVALID_ARGUMENT);
    }

    auto packed_r = packer_.tryPackOne(image->getExtent());
    if (!packed_r) {
        return getOrCreateNextAtlas().copyOnFrame(id, image);
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
    if (!pending_data_.empty()) {
        auto pd_it = pending_data_.begin();
        while (pd_it != pending_data_.end()) {
            image_->copyBuffer(Renderer::i().frameRenderer().cmd(), *std::get<BufferHnd>(*pd_it), std::get<Recti>(*pd_it));
            Renderer::i().frameRenderer().deleteAfterFrame(std::move(std::get<BufferHnd>(*pd_it)));
            pd_it++;
        }

        pending_data_.clear();

        image_->transitionState(Renderer::i().frameRenderer().cmd(), Image::State::FS_READ);
    }
    if (next_atlas_) {
        next_atlas_->updateOnFrame();
    }
}

void TextureAtlas::requestFlushOnFrame() {
    if (flush_requested_) {
        return;
    }
    Renderer::i().frameRenderer().callOnNextFrameBegin([this]() {
        updateOnFrame();
        flush_requested_ = false;
    });
    flush_requested_ = true;
}
}  // namespace mle
