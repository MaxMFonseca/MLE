#include "mle/renderer/Buffer.h"

#include "MeshCache.h"

namespace mle {
MeshRef MeshCache::add(entt::id_type id, const GLTF& gltf) {
    return add(id, gltf, max<usize>());
}

MeshRef MeshCache::add(entt::id_type id, const GLTF& gltf, usize root_node) {
    if (auto found = meshes_.find(id); found != meshes_.end()) {
        return found->second.get();
    }

    auto model = std::make_unique<Mesh>();
    if (root_node == max<usize>()) {
        model->init(gltf);
    } else {
        model->init(gltf, root_node);
    }

    auto [it, inserted] = meshes_.emplace(id, std::move(model));
    MLE_D("Added model id:{} to MeshCache", id);
    return it->second.get();
}

MeshRef MeshCache::addMesh(const std::string& model_name) {
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
    MLE_D("Added model '{}' to MeshCache from file: {}", model_name, path.string());
    return model;
};

MeshRef MeshCache::get(entt::id_type id) {
    if (auto alias_it = aliases_.find(id); alias_it != aliases_.end()) {
        id = alias_it->second;
    }
    auto it = meshes_.find(id);
    if (it != meshes_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool MeshCache::contains(entt::id_type id) const {
    if (auto alias_it = aliases_.find(id); alias_it != aliases_.end()) {
        id = alias_it->second;
    }
    return meshes_.contains(id);
}

void MeshCache::addAlias(entt::id_type alias_id, entt::id_type target_id) {
    aliases_[alias_id] = target_id;
}

void MeshCache::shutdown() {
    meshes_.clear();
}
}  // namespace mle
