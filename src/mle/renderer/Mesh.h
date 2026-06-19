#pragma once

#include <tiny_gltf.h>

#include <optional>
#include <span>
#include <string>
#include <vector>

#include "mle/math/Types.h"
#include "mle/math/Types2D.h"
#include "mle/renderer/GLTF.h"
#include "mle/renderer/Primitive.h"
#include "mle/renderer/SkinBinding.h"

namespace mle {
class Mesh {
  public:
    struct Node {
        std::string name;
        int parent = -1;
        vec3f base_translation{};
        quat base_rotation{};
        vec3f base_scale{};
        bool included = true;
    };

    struct NodePrimitive {
        usize node_index = 0;
        int skin_index = -1;
        Primitive primitive;
    };

  public:
    MLE_NO_COPY_MOVE(Mesh);

    Mesh() = default;
    ~Mesh();

    void init(const GLTF& gltf);
    void init(const GLTF& gltf, usize root_node);

    [[nodiscard]] const std::vector<Node>& getNodes() const { return nodes_; }
    [[nodiscard]] auto getNodeCount() const { return nodes_.size(); }
    [[nodiscard]] const Node& getNode(usize index) const { return nodes_.at(index); }
    [[nodiscard]] usize getNodeIdxByName(const std::string& name) const;
    [[nodiscard]] const auto& getPrimitive() const { return primitives_.at(0).primitive; }
    [[nodiscard]] std::optional<const Primitive*> getPrimitiveByName(const std::string& name) const;
    [[nodiscard]] const auto& getPrimitives() const { return primitives_; }
    [[nodiscard]] const std::vector<SkinBinding>& getSkins() const { return skins_; }

    void evaluateBase(std::span<mat4f> out_node_globals) const;

    [[nodiscard]] const std::vector<usize>& getEvaluationOrder() const { return evaluation_order_; }

    [[nodiscard]] Polygon2f projectPolygon(Axis axis, f32 offset = 0.0F) const;

  private:
    std::vector<Node> nodes_;
    std::vector<NodePrimitive> primitives_;
    std::vector<SkinBinding> skins_;
    std::vector<usize> evaluation_order_;
};
}  // namespace mle
