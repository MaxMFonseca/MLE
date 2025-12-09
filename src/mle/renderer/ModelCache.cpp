#include "ModelCache.h"

#include "mle/renderer/Buffer.h"

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

MeshRef ModelCache::getMesh(const std::string& mesh_name) {
    auto it = meshes_.find(mesh_name);
    if (it != meshes_.end()) {
        return it->second.get();
    }
    return nullptr;
};

void ModelCache::addMeshes(const std::string& gltf_path) {
    Path path = ResPath::RES;
    path /= ResPath::MODELS;
    path /= gltf_path;

    GLTF gltf;

    if (gltf.load(path) != Result::OK) {
        MLE_E("Failed to load model for meshes: {}", path.string());
        return;
    }

    const auto& model = gltf.model();
    MLE_D("Found {} meshes in file: {}", model.meshes.size(), path.string());
    for (usize mid = 0; mid < model.meshes.size(); ++mid) {
        const auto& src_mesh = model.meshes[mid];
        MLE_ASSERT_LOG(src_mesh.name.empty() == false, "Mesh in gltf has no name, cannot add to ModelCache: {}", path.string());
        auto name = gltf_path + "." + src_mesh.name;
        auto mesh_i = meshes_.emplace(name, std::make_unique<Mesh>());
        MLE_ASSERT_LOG(mesh_i.second, "Mesh '{}' already exists in ModelCache", name);
        auto& mesh = mesh_i.first->second;
        mesh->load(gltf, mid);
        MLE_D("Added mesh '{}' to ModelCache from file: {}", src_mesh.name, path.string());
    }
};

MeshRef ModelCache::addMesh(const GLTF& gltf, usize mesh_index) {
    const auto& model = gltf.model();
    MLE_ASSERT_LOG(mesh_index < model.meshes.size(), "Mesh index out of bounds in ModelCache::addMesh");
    const auto& src_mesh = model.meshes[mesh_index];
    MLE_ASSERT_LOG(src_mesh.name.empty() == false, "Mesh in gltf has no name, cannot add to ModelCache");
    auto name = src_mesh.name;
    auto mesh_i = meshes_.emplace(name, std::make_unique<Mesh>());
    MLE_ASSERT_LOG(mesh_i.second, "Mesh '{}' already exists in ModelCache", name);
    auto& mesh = mesh_i.first->second;
    mesh->load(gltf, mesh_index);
    MLE_D("Added mesh '{}' to ModelCache", src_mesh.name);
    return mesh.get();
};
}  // namespace mle
