#pragma once

#include <string>
#include <unordered_map>

#include "mle/renderer/Model.h"

namespace mle {
class ModelCache {
  public:
    MLE_NO_COPY_MOVE(ModelCache);

    ~ModelCache() = default;

    ModelRef addModel(const std::string& model_name);
    ModelRef getModel(const std::string& model_name);

    MeshRef addMesh(const GLTF& gltf, usize mesh_index);
    void addMeshes(const std::string& gltf_path);
    MeshRef getMesh(const std::string& mesh_name);

  private:
    friend class Renderer;
    ModelCache() = default;
    void init() {};
    void shutdown() {};

  private:
    std::unordered_map<std::string, ModelHnd> models_;
    std::unordered_map<std::string, MeshHnd> meshes_;
};
}  // namespace mle
