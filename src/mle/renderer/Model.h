#pragma once

#include <tiny_gltf.h>

#include <string>
#include <vector>

#include "mle/math/Types.h"
#include "mle/renderer/GLTF.h"
#include "mle/renderer/Mesh.h"

namespace mle {
class Model {
  public:
    struct Node {
        std::string name;
        int parent = -1;
        vec3f base_translation{};
        quat base_rotation{};
        vec3f base_scale{};
        bool included = true;
    };

    struct NodeMesh {
        usize node_index = 0;
        int skin_index = -1;
        Mesh mesh;
    };

  public:
    MLE_NO_COPY_MOVE(Model);

    Model() = default;
    ~Model();

    void init(const GLTF& gltf);
    void init(const GLTF& gltf, usize root_node);

    [[nodiscard]] const std::vector<Node>& getNodes() const { return nodes_; }
    [[nodiscard]] auto getNodeCount() const { return nodes_.size(); }
    [[nodiscard]] const Node& getNode(usize index) const { return nodes_.at(index); }
    [[nodiscard]] usize getNodeIdxByName(const std::string& name) const;
    [[nodiscard]] const auto& getMesh() const { return meshes_.at(0).mesh; }
    [[nodiscard]] const auto& getMeshes() const { return meshes_; }

    void evaluateBase(std::vector<mat4f>& out_node_globals) const;

  private:
    std::vector<Node> nodes_;
    std::vector<NodeMesh> meshes_;
};
}  // namespace mle
