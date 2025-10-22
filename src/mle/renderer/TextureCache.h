#pragma once

#include <unordered_map>

#include "Types.h"
#include "mle/renderer/Image.h"
#include "mle/utils/ECS.h"

namespace mle {
class TextureCache {
  public:
    void init();
    void shutdown();

    Expected<ImageRef> get(entt::id_type id);
    Expected<vec2u> getExtent(entt::id_type id);

    void addTexture(entt::id_type id, ImageHnd&& img);
    void addTexture(entt::id_type id, const Image::RawData& raw_data);
    ImageRef addTextureWait(entt::id_type id, const Image::RawData& raw_data);

    Expected<ImageRef> loadTexture(const std::string& src);
    void loadTextures(std::span<const std::string> names);

    void setSampler(entt::id_type id, vk::SamplerCreateInfo sampler_ci);
    vk::Sampler getSampler(entt::id_type id = 0) const;

  private:
    [[nodiscard]] BufferHnd createTexture(CommandBuffer& cmd, entt::id_type id, const Image::RawData& raw_data);

  private:
    struct TextureData {
        ImageHnd image{};
        bool ready = false;
    };

    std::mutex mutex_;
    std::unordered_map<entt::id_type, TextureData> textures_;
    ImageRef default_texture_{nullptr};

    std::unordered_map<entt::id_type, vk::Sampler> samplers_;
    vk::Sampler default_sampler_{nullptr};
};
}  // namespace mle
