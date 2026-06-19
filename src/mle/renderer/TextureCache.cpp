#include "TextureCache.h"

#include "mle/renderer/Buffer.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Renderer.h"
#include "mle/utils/ECS.h"

namespace mle {
void TextureCache::init() {
    vk::SamplerCreateInfo sampler_ci{};
    sampler_ci.magFilter = vk::Filter::eLinear;
    sampler_ci.minFilter = vk::Filter::eLinear;
    sampler_ci.addressModeU = vk::SamplerAddressMode::eRepeat;
    sampler_ci.addressModeV = vk::SamplerAddressMode::eRepeat;
    sampler_ci.addressModeW = vk::SamplerAddressMode::eRepeat;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.borderColor = vk::BorderColor::eIntOpaqueBlack;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = vk::CompareOp::eAlways;
    sampler_ci.mipmapMode = vk::SamplerMipmapMode::eLinear;
    sampler_ci.mipLodBias = 0.0F;
    sampler_ci.minLod = 0.0F;
    sampler_ci.maxLod = VK_LOD_CLAMP_NONE;

    setSampler(entt::hashed_string{"default"}, sampler_ci);

    Image::RawData default_image_data{};
    default_image_data.extent = {10, 10};
    default_image_data.channels = 4;
    default_image_data.pixels.resize(as<u64>(default_image_data.extent.x) * default_image_data.extent.y * default_image_data.channels, 255);

    for (u32 y = 0; y < default_image_data.extent.y; ++y) {
        for (u32 x = 0; x < default_image_data.extent.x; ++x) {
            auto idx = (y * default_image_data.extent.x + x) * default_image_data.channels;
            if ((x / 5 + y / 5) % 2 == 0) {
                default_image_data.pixels[idx + 0] = 0;
                default_image_data.pixels[idx + 1] = 0;
                default_image_data.pixels[idx + 2] = 0;
            } else {
                default_image_data.pixels[idx + 0] = 0;
                default_image_data.pixels[idx + 1] = 255;
                default_image_data.pixels[idx + 2] = 255;
            }
        }
    }

    default_texture_ = addTextureWait(entt::hashed_string{"default"}, default_image_data);

    auto make_solid = [](u8 r, u8 g, u8 b, u8 a, bool srgb) {
        Image::RawData data{};
        data.extent = {1, 1};
        data.channels = 4;
        data.srgb = srgb;
        data.pixels = {r, g, b, a};
        return data;
    };

    white_texture_ = addTextureWait(entt::hashed_string{"mle_default_white"}, make_solid(255, 255, 255, 255, false));
    black_texture_ = addTextureWait(entt::hashed_string{"mle_default_black"}, make_solid(0, 0, 0, 255, false));
    flat_normal_texture_ = addTextureWait(entt::hashed_string{"mle_default_flat_normal"}, make_solid(128, 128, 255, 255, false));
}

void TextureCache::shutdown() {
    MLE_I("Shutting down texture cache");
    {
        std::scoped_lock lock(mutex_);
        textures_.clear();
        retired_textures_.clear();
    }
    for (auto& [name, sampler] : samplers_) {
        Renderer::i().destroy(sampler);
    }
    samplers_.clear();
}

void TextureCache::setSampler(entt::id_type id, vk::SamplerCreateInfo sampler_ci) {
    auto sampler = unwrap(Renderer::i().vkDevice().createSampler(sampler_ci));
    if (auto found = samplers_.find(id); found != samplers_.end()) {
        Renderer::i().destroy(found->second);
        found->second = sampler;
    } else {
        samplers_.emplace(id, sampler);
    }
    if (id == entt::hashed_string{"default"}) {
        default_sampler_ = sampler;
    }
}

vk::Sampler TextureCache::getSampler(entt::id_type id) const {
    if (id == 0 || id == entt::hashed_string{"default"}) {
        return default_sampler_;
    }
    if (auto found = samplers_.find(id); found != samplers_.end()) {
        return found->second;
    }
    MLE_W("Sampler id:{} not found, returning default sampler", id);
    return default_sampler_;
}

Expected<ImageRef> TextureCache::get(entt::id_type id) {
    if (id == 0 || id == entt::hashed_string{"default"}) {
        return default_texture_;
    }

    std::scoped_lock lock(mutex_);
    auto found = textures_.find(id);
    if (found != textures_.end()) {
        if (found->second.ready) {
            return found->second.image.get();
        }
        MLE_T("Texture id:{} is not ready yet", id);
        return std::unexpected(Result::NOT_READY);
    }
    MLE_E("Texture id:{} not found", id);
    return std::unexpected(Result::NOT_FOUND);
};

bool TextureCache::contains(entt::id_type id) const {
    if (id == 0 || id == entt::hashed_string{"default"}) {
        return default_texture_ != nullptr;
    }
    std::scoped_lock lock(mutex_);
    return textures_.contains(id);
}

Expected<vec2u> TextureCache::getExtent(entt::id_type id) {
    if (id == 0 || id == entt::hashed_string{"default"}) {
        return default_texture_->getExtent();
    }

    std::scoped_lock lock(mutex_);
    auto found = textures_.find(id);
    if (found != textures_.end()) {
        return found->second.image->getExtent();
    }
    MLE_E("Texture id:{} not found", id);
    return std::unexpected(Result::NOT_FOUND);
};

void TextureCache::addTexture(entt::id_type id, ImageHnd&& img) {
    TextureData data;
    data.image = std::shared_ptr<Image>(std::move(img));
    data.ready = true;
    std::scoped_lock lock(mutex_);
    setTextureLocked(id, std::move(data));
}

void TextureCache::setTextureLocked(entt::id_type id, TextureData&& data) {
    auto found = textures_.find(id);
    if (found != textures_.end()) {
        if (found->second.image != nullptr) {
            retired_textures_.push_back(found->second.image);
        }
        found->second = std::move(data);
        return;
    }
    textures_.emplace(id, std::move(data));
}

void TextureCache::markReady(entt::id_type id, const std::shared_ptr<Image>& image) {
    std::scoped_lock lock(mutex_);
    auto found = textures_.find(id);
    if (found == textures_.end()) {
        MLE_E("Texture id:{} not found while marking ready", id);
        return;
    }
    if (found->second.image != image) {
        return;
    }
    found->second.ready = true;
}

TextureCache::PendingTextureUpload TextureCache::createTexture(CommandBuffer& cmd, entt::id_type id, const Image::RawData& raw_data) {
    Image::CI ci{};
    ci.extent = raw_data.extent;

    switch (raw_data.channels) {
        case 1:
            ci.format = Image::Format::TEXTURE_1U;
            break;
        case 2:
            ci.format = Image::Format::TEXTURE_2U;
            break;
        case 4:
            ci.format = raw_data.srgb ? Image::Format::TEXTURE_4SRGB : Image::Format::TEXTURE_4U;
            break;
        default:
            MLE_E("Unsupported number of channels: {}", raw_data.channels);
            return {};
    }

    auto image = std::shared_ptr<Image>(Image::createHnd(ci));

    auto sb = image->copyRaw(cmd, raw_data);
    image->ownershipRelease(cmd, Renderer::i().commandManager().queueDataIdx(GCmdType::GRAPHICS));

    {
        std::scoped_lock lock(mutex_);
        setTextureLocked(id, TextureData{.image = image, .ready = false});
    }

    return {.staging_buffer = std::move(sb), .image = std::move(image)};
}

void TextureCache::addTexture(entt::id_type id, const Image::RawData& raw_data) {
    auto& cmd_mgr = Renderer::i().commandManager();
    auto cmd = cmd_mgr.getOTS(GCmdType::TRANSFER);

    auto upload = createTexture(cmd, id, raw_data);

    cmd_mgr.submitOTSAsync(std::move(cmd), {}, [this, id, staging_buffer = std::move(upload.staging_buffer), image = std::move(upload.image)]() {
        if (image == nullptr) {
            return;
        }

        auto& cmd_mgr = Renderer::i().commandManager();
        auto gcmd = cmd_mgr.getOTS(GCmdType::GRAPHICS);
        image->ownershipAcquire(gcmd);
        image->transitionState(gcmd, Image::State::FS_READ);
        cmd_mgr.submitOTSWait(std::move(gcmd));
        markReady(id, image);
    });
}

ImageRef TextureCache::addTextureWait(entt::id_type id, const Image::RawData& raw_data) {
    auto& cmd_mgr = Renderer::i().commandManager();
    auto cmd = cmd_mgr.getOTS(GCmdType::TRANSFER);

    auto upload = createTexture(cmd, id, raw_data);

    cmd_mgr.submitOTSWait(std::move(cmd));

    if (upload.image == nullptr) {
        return nullptr;
    }

    auto gcmd = cmd_mgr.getOTS(GCmdType::GRAPHICS);
    upload.image->ownershipAcquire(gcmd);
    upload.image->transitionState(gcmd, Image::State::FS_READ);
    cmd_mgr.submitOTSWait(std::move(gcmd));

    markReady(id, upload.image);

    return upload.image.get();
}

Expected<ImageRef> TextureCache::loadTexture(const std::string& src, bool srgb) {
    auto id = entt::hashed_string{src.c_str()};
    {
        std::scoped_lock lock(mutex_);
        if (const auto found = textures_.find(id); found != textures_.end()) {
            if (found->second.ready) {
                return found->second.image.get();
            }
            return std::unexpected(Result::NOT_READY);
        }
    }

    auto raw_data_r = Image::readFile("res/textures/" + src, 4);

    if (!raw_data_r.has_value()) {
        MLE_E("Failed to load texture {}: {}", src, raw_data_r.error());
        return std::unexpected(raw_data_r.error());
    }
    raw_data_r.value().srgb = srgb;

    addTexture(id, raw_data_r.value());
    return std::unexpected(Result::NOT_READY);
}

void TextureCache::loadTextures(std::span<const std::string> names) {
    auto& cmd_mgr = Renderer::i().commandManager();
    auto cmd = cmd_mgr.getOTS(GCmdType::TRANSFER);

    std::vector<BufferHnd> staging_buffers;
    std::vector<std::shared_ptr<Image>> images;
    std::vector<entt::id_type> ids;
    for (const auto& name : names) {
        auto id = entt::hashed_string{name.c_str()};
        {
            std::scoped_lock lock(mutex_);
            if (textures_.contains(id)) {
                MLE_W("Texture src:{} is already loaded", name);
                continue;
            }
        }

        auto raw_data_r = Image::readFile("res/textures/" + name);

        if (!raw_data_r.has_value()) {
            MLE_E("Failed to load texture {}: {}", name, raw_data_r.error());
            continue;
        }

        auto upload = createTexture(cmd, id, raw_data_r.value());
        if (upload.image == nullptr) {
            continue;
        }
        staging_buffers.push_back(std::move(upload.staging_buffer));
        images.push_back(std::move(upload.image));
        ids.push_back(id);
    }

    cmd_mgr.submitOTSAsync(std::move(cmd), {}, [this, ids = std::move(ids), staging_buffers = std::move(staging_buffers), images = std::move(images)]() {
        if (images.empty()) {
            return;
        }

        auto& cmd_mgr = Renderer::i().commandManager();
        auto gcmd = cmd_mgr.getOTS(GCmdType::GRAPHICS);
        for (const auto& image : images) {
            image->ownershipAcquire(gcmd);
            image->transitionState(gcmd, Image::State::FS_READ);
        }
        cmd_mgr.submitOTSWait(std::move(gcmd));
        for (usize i = 0; i < ids.size(); ++i) {
            markReady(ids[i], images[i]);
        }
    });
}
}  // namespace mle
