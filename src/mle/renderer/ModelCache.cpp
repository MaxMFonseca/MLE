#include "mle/renderer/Buffer.h"

#include "ModelCache.h"

namespace mle {
ModelRef ModelCache::add(entt::id_type id, const GLTF& gltf) {
    if (auto found = models_.find(id); found != models_.end()) {
        return found->second.get();
    }

    auto model = std::make_unique<Model>();
    model->init(gltf);

    auto [it, inserted] = models_.emplace(id, std::move(model));
    MLE_D("Added model id:{} to ModelCache", id);
    return it->second.get();
}

ModelRef ModelCache::addModel(const std::string& model_name) {
    const auto id = entt::hashed_string::value(model_name.c_str());
    if (auto* model = get(id); model != nullptr) {
        return model;
    }

    Path path = ResPath::RES;
    path /= ResPath::MODELS;
    path /= model_name;

    GLTF gltf;

    if (gltf.load(path) != Result::OK) {
        MLE_E("Failed to load model: {}", path.string());
        return nullptr;
    }

    auto* model = add(id, gltf);
    MLE_D("Added model '{}' to ModelCache from file: {}", model_name, path.string());
    return model;
};

ModelRef ModelCache::get(entt::id_type id) {
    auto it = models_.find(id);
    if (it != models_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool ModelCache::contains(entt::id_type id) const {
    return models_.contains(id);
}

void ModelCache::shutdown() {
    models_.clear();
}
}  // namespace mle
