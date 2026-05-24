#include "Utils.h"

#include "mle/utils/File.h"

namespace mle {
bool isSrgbFormat(vk::Format format) {
    switch (format) {
        case vk::Format::eR8Srgb:
        case vk::Format::eR8G8Srgb:
        case vk::Format::eR8G8B8Srgb:
        case vk::Format::eB8G8R8Srgb:
        case vk::Format::eR8G8B8A8Srgb:
        case vk::Format::eB8G8R8A8Srgb:
        case vk::Format::eA8B8G8R8SrgbPack32:
            return true;
        default:
            return false;
    }
}
}  // namespace mle
