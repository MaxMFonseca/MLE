#pragma once

#include <filesystem>
#include <list>
#include <unordered_map>

#include "mle/common/Utils.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Types.h"

// This is a very simple cache that will be used with descriptor indexing
// Indexes should NEVER be invalidated
namespace mle::renderer::detail {
class TextureCache final {
  public:
    MLE_NO_COPY_MOVE(TextureCache)

    TextureCache() = default;
    ~TextureCache() = default;

    void init();
    void reset();

    void update();

    Texture add(const std::string& name, bool engine = false);
    Texture add(const fs::path& path, std::string name);
    Texture get(const std::string& name, bool engine = false);

  private:
    void finishedUpload(u32 idx);

  private:
    struct TextureData {
        ImageHnd image{};
        bool ready = false;
    };
    std::vector<TextureData> textures_;
    std::unordered_map<std::string, u32> texture_names_;
    std::list<u32> free_indices_;

    struct UpdatingData {
        BufferHnd staging_buffer;
        vk::Semaphore semaphore;
        u32 idx;
    };
    std::list<UpdatingData> updating_textures_;
};
}  // namespace mle::renderer::detail
