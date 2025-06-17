#include "TextureCache.h"

namespace mle::renderer::detail {
void TextureCache::init() {  // NOLINT
    MLE_I("Initializing texture cache");
}

void TextureCache::reset() {
    MLE_I("Shutting down texture cache");
    textures_.clear();
}

Texture TextureCache::add(const std::string& name, bool engine) {
    fs::path path = name;
    if (engine) {
        path = res::addMleTexturePath(name);
    } else {
        path = res::addUserTexturePath(name);
    }
    return add(path, name);
}

Texture TextureCache::add(const fs::path& path, std::string name) {
    MLE_D("Adding texture from file: {}", path.generic_string());

    MLE_ASSERT(!textures_.contains(name));

    if (name.empty()) {
        name = res::removeBasePath(path.string());
        MLE_D("Texture name not provided, using: {}", name);
    } else {
        MLE_D("Texture name provided: {}", name);
    }

    auto file_data = Image::readFile(path);

    TextureData td = {.idx = index_++};
    Image::CI ci;
    ci.extent = file_data.extent;
    ci.format = Image::getDefaultFormatForChannelCount(file_data.channels);
    ci.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
    td.image = Image::createHnd(ci);

    auto& ret = textures_[name] = std::move(td);

    return {.image = ret.image.get(), .idx = ret.idx};
}

Texture TextureCache::get(const std::string& name, bool engine) {
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        return {.image = it->second.image.get(), .idx = it->second.idx};
    }

    return add(name, engine);  // If not found, try to add it
}

}  // namespace mle::renderer::detail
