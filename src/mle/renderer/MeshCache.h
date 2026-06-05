#pragma once

#include <string>
#include <unordered_map>

#include "mle/renderer/GLTF.h"
#include "mle/renderer/Mesh.h"
#include "mle/utils/ECS.h"

namespace mle {
class MeshCache {
  public:
    MLE_NO_COPY_MOVE(MeshCache);

    ~MeshCache() = default;

    MeshRef add(entt::id_type id, const GLTF& gltf);
    MeshRef add(entt::id_type id, const GLTF& gltf, usize root_node);
    MeshRef addMesh(const std::string& model_name);
    MeshRef get(entt::id_type id);
    [[nodiscard]] bool contains(entt::id_type id) const;
    void addAlias(entt::id_type alias_id, entt::id_type target_id);

  private:
    friend class Renderer;
    MeshCache() = default;
    void init() {};
    void shutdown();

  private:
    std::unordered_map<entt::id_type, MeshHnd> meshes_;
    std::unordered_map<entt::id_type, entt::id_type> aliases_;
};
}  // namespace mle
