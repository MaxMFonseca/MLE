#pragma once

#include <unordered_map>

#include "mle/common/Result.h"
#include "mle/common/math/Types.h"
#include "mle/renderer/Image.h"

namespace mle::renderer {
class TextureAtlas final {
  public:
    struct GroupCreateInfo {
        std::string name;
        vec2i atlas_extent;
        vec2i atlas_padding;
        vk::Format format;
    };

    using TexturePack = std::vector<std::pair<std::string, Image::RawData>>;

  public:
    TextureAtlas(const TextureAtlas&) = delete;
    TextureAtlas(TextureAtlas&&) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;
    TextureAtlas& operator=(TextureAtlas&&) = delete;

    explicit TextureAtlas(const GroupCreateInfo& default_group_ci);
    ~TextureAtlas() = default;

    Result update();

    Texture addTexture(const std::string& name, const Image::RawData& data, const std::string& group = "");
    void addTextures(const TexturePack& texture_pack, const std::string& group = "");
    void addTexturesPacked(const TexturePack& texture_pack, vec2u padding = {0, 0}, vec2u max_extent = {0, 0}, const std::string& group = "");

    void addGroup(const GroupCreateInfo& ci);

    [[nodiscard]] Texture getTexture(const std::string& name, const std::string& group = "");
    [[nodiscard]] auto getImageCount() const { return images_.size(); }
    [[nodiscard]] usize getImageCountGroup(const std::string& group);
    [[nodiscard]] WriteDescriptorSet makeWriteDescriptorSet(usize binding, vk::Sampler sampler, const Texture& texture);
    [[nodiscard]] WriteDescriptorSet makeWriteDescriptorSet(usize binding, vk::Sampler sampler, const std::string& group);
    [[nodiscard]] WriteDescriptorSet makeWriteDescriptorSet(usize binding, vk::Sampler sampler);

  private:
    struct GroupData {
        std::string name;
        std::unordered_map<std::string, Texture> textures;
        std::vector<u32> image_indices;
        vec2i atlas_extent;
        vec2i atlas_padding;
        int max_height_for_lazy = 0;
        vk::Format atlas_format;
    };
    struct ImageData {
        ImageHnd image;
        vec2i cur_pos = {0, 0};
        int next_y = 0;
        bool is_atlas = false;
    };

    [[nodiscard]] GroupData& getGroup(const std::string& name);

    ImageData& createImage(vec2i extent, vk::Format format, GroupData& group_data);
    ImageData& createAtlas(GroupData& group_data);
    Texture addDedicatedTexture(const std::string& name, const Image::RawData& data, GroupData& group_data);
    Texture addTextureToAtlas(const std::string& name, const Image::RawData& data, GroupData& group_data, usize idx);

    Texture makeDefaultTexture();

  private:
    std::vector<ImageData> images_;
    bool image_count_changed_ = false;

    std::vector<GroupData> groups_;

    struct AddQueueEntry {
        BufferHnd buffer;
        usize img_idx;
        Recti rect;
    };
    std::vector<AddQueueEntry> add_queue_;
};
}  // namespace mle::renderer
