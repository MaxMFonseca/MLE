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
}

void TextureCache::shutdown() {
    MLE_I("Shutting down texture cache");
    textures_.clear();
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

void TextureCache::addTexture(entt::id_type id, ImageHnd&& img) {
    TextureData data;
    data.image = std::move(img);
    data.ready = true;
    textures_.emplace(id, std::move(data));
}

BufferHnd TextureCache::createTexture(CommandBuffer& cmd, entt::id_type id, const Image::RawData& raw_data) {
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
            ci.format = Image::Format::TEXTURE_4U;
            break;
        default:
            MLE_E("Unsupported number of channels: {}", raw_data.channels);
            return {};
    }

    ImageHnd img = Image::createHnd(ci);

    auto sb = img->copyRaw(cmd, raw_data);
    img->ownershipRelease(cmd, Renderer::i().commandManager().queueDataIdx(GCmdType::GRAPHICS));

    {
        std::scoped_lock lock(mutex_);
        textures_[id] = TextureData{.image = std::move(img), .ready = false};
    }

    return sb;
}

void TextureCache::addTexture(entt::id_type id, const Image::RawData& raw_data) {
    auto& cmd_mgr = Renderer::i().commandManager();
    auto cmd = cmd_mgr.getOTS(GCmdType::TRANSFER);

    auto staging_buffer = createTexture(cmd, id, raw_data);

    cmd_mgr.submitOTSAsync(std::move(cmd), {}, [this, id, staging_buffer = std::move(staging_buffer)]() {
        auto& cmd_mgr = Renderer::i().commandManager();
        auto gcmd = cmd_mgr.getOTS(GCmdType::GRAPHICS);
        textures_[id].image->ownershipAcquire(gcmd);
        textures_[id].image->transitionState(gcmd, Image::State::FS_READ);
        cmd_mgr.submitOTSWait(std::move(gcmd));
        {
            std::scoped_lock lock(mutex_);
            textures_[id].ready = true;
        }
    });
}

Expected<ImageRef> TextureCache::loadTexture(const std::string& src) {
    auto id = entt::hashed_string{src.c_str()};
    if (const auto found = textures_.find(id); found != textures_.end()) {
        if (found->second.ready) {
            return found->second.image.get();
        }
        return std::unexpected(Result::NOT_READY);
    }

    auto raw_data_r = Image::readFile("res/textures/" + src);

    if (!raw_data_r.has_value()) {
        MLE_E("Failed to load texture {}: {}", src, raw_data_r.error());
        return std::unexpected(raw_data_r.error());
    }

    addTexture(id, raw_data_r.value());
    return std::unexpected(Result::NOT_READY);
}

void TextureCache::loadTextures(std::span<const std::string> names) {
    auto& cmd_mgr = Renderer::i().commandManager();
    auto cmd = cmd_mgr.getOTS(GCmdType::TRANSFER);

    std::vector<BufferHnd> staging_buffers;
    std::vector<entt::id_type> ids;
    for (const auto& name : names) {
        auto id = entt::hashed_string{name.c_str()};
        if (textures_.contains(id)) {
            MLE_W("Texture src:{} is already loaded", name);
            continue;
        }

        auto raw_data_r = Image::readFile("res/textures/" + name);

        if (!raw_data_r.has_value()) {
            MLE_E("Failed to load texture {}: {}", name, raw_data_r.error());
            continue;
        }

        staging_buffers.push_back(createTexture(cmd, id, raw_data_r.value()));
        ids.push_back(id);
    }

    cmd_mgr.submitOTSAsync(std::move(cmd), {}, [this, ids = std::move(ids), staging_buffers = std::move(staging_buffers)]() {
        auto& cmd_mgr = Renderer::i().commandManager();
        auto gcmd = cmd_mgr.getOTS(GCmdType::GRAPHICS);
        for (const auto& id : ids) {
            textures_[id].image->ownershipAcquire(gcmd);
            textures_[id].image->transitionState(gcmd, Image::State::FS_READ);
        }
        cmd_mgr.submitOTSWait(std::move(gcmd));
        {
            std::scoped_lock lock(mutex_);
            for (const auto& id : ids) {
                textures_[id].ready = true;
            }
        }
    });
}
}  // namespace mle
