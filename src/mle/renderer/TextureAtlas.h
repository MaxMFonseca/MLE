#pragma once

#include <map>

#include "mle/renderer/Buffer.h"
#include "mle/renderer/Image.h"
#include "mle/utils/ECS.h"
#include "mle/utils/RectPacker.h"
#include "mle/utils/Utils.h"

namespace mle {
class TextureAtlas {
  public:
    struct CreateInfo {
        Image::Format format;
        vec2u extent;
        vec2u padding{2, 2};
    };
    using CI = CreateInfo;

    using Entry = Rectf;

  public:
    TextureAtlas() = default;
    ~TextureAtlas() = default;
    MLE_NO_COPY_MOVE(TextureAtlas);

    void init(const CreateInfo& ci);

    [[nodiscard]] Expected<std::pair<ImageRef, Entry>> get(entt::id_type id) const;

    void enqueueCopy(entt::id_type id, const Image::RawData& data);
    void enqueueCopy(entt::id_type id, vec2u vec, BufferHnd&& buffer);

    Expected<std::pair<ImageRef, Entry>> copyOnFrame(entt::id_type id, const Image::RawData& data);
    Expected<std::pair<ImageRef, Entry>> copyOnFrame(entt::id_type id, ImageRef image);

    void updateOnFrame();

  private:
    void createChildAtlas();

  private:
    ImageHnd image_{nullptr};

    std::map<entt::id_type, Entry> entries_;
    std::vector<std::tuple<entt::id_type, vec2u, BufferHnd>> pending_data_;

    TextureAtlasHnd next_atlas_{nullptr};
    RectPacker packer_;
};
}  // namespace mle
