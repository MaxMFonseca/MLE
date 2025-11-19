#include "Utils.h"

#include "mle/utils/File.h"

namespace mle {
Expected<tinygltf::Model> loadGltfFromFile(const std::string& path) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;

    Path fs_path = path;

    if (!std::filesystem::exists(fs_path)) {
        MLE_E("GLTF file does not exist: {}", path);
        return std::unexpected(Result::FAILED_TO_OPEN);
    }

    bool is_binary = fs_path.extension() == ".glb";

    bool ok = false;

    if (is_binary) {
        ok = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    } else {
        ok = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    }

    if (!warn.empty()) {
        MLE_W("GLTF warning: {}", warn);
    }
    if (!ok) {
        MLE_E("Failed to load GLTF file: {}", err);
        return std::unexpected(Result::FAILED_TO_OPEN);
    }

    return model;
};
}  // namespace mle
