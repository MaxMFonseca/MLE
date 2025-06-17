#pragma once

#include <filesystem>
#include <unordered_map>

#include "mle/common/Utils.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/Types.h"

// This is a very simple cache that will be used with descriptor indexing
namespace mle::renderer::detail {
class TextureCache final {
  public:
    MLE_NO_COPY_MOVE(TextureCache)

    TextureCache() = default;
    ~TextureCache() = default;

    void init();
    void reset();

    Texture add(const std::string& name, bool engine = false);
    Texture add(const fs::path& path, std::string name);
    Texture get(const std::string& name, bool engine = false);

  private:
    struct TextureData {
        ImageHnd image{};
        u32 idx{};
    };
    u32 index_ = 0;
    std::unordered_map<std::string, TextureData> textures_;
};
}  // namespace mle::renderer::detail
