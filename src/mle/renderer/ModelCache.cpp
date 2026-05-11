#include "mle/renderer/Buffer.h"

#include "ModelCache.h"

namespace mle {
ModelRef ModelCache::addModel(const std::string& model_name) {
    Path path = ResPath::RES;
    path /= ResPath::MODELS;
    path /= model_name;

    GLTF gltf;

    if (gltf.load(path) != Result::OK) {
        MLE_E("Failed to load model: {}", path.string());
        return nullptr;
    }

    auto model_i = models_.emplace(model_name, std::make_unique<Model>());
    auto& model = model_i.first->second;
    model->init(gltf);
    MLE_D("Added model '{}' to ModelCache from file: {}", model_name, path.string());
    return model.get();
};

ModelRef ModelCache::getModel(const std::string& model_name) {
    auto it = models_.find(model_name);
    if (it != models_.end()) {
        return it->second.get();
    }
    return nullptr;
}
}  // namespace mle
