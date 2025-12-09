#pragma once

#include <tiny_gltf.h>

#include <string>
#include <vector>

#include "mle/math/Types.h"
#include "mle/renderer/GLTF.h"
#include "mle/renderer/Mesh.h"
#include "mle/renderer/Skeleton.h"

namespace mle {
class Model {
  public:
    struct Node {
        std::string name;
        int parent = -1;
        vec3f base_translation{};
        quat base_rotation{};
        vec3f base_scale{};
    };

    struct KeyframeVec3 {
        f32 time = 0.0F;
        vec3f value{};
    };

    struct KeyframeQuat {
        f32 time = 0.0F;
        quat value{};
    };

    struct NodeAnim {
        std::vector<KeyframeVec3> translations;
        std::vector<KeyframeQuat> rotations;
        std::vector<KeyframeVec3> scales;
    };

    struct Animation {
        std::string name;
        f32 duration = 0.0F;
        std::vector<NodeAnim> nodes;
    };

    struct NodeMesh {
        usize node_index = 0;
        Mesh mesh;
    };

  public:
    void init(const GLTF& gltf);

    [[nodiscard]] const std::vector<Node>& getNodes() const { return nodes_; }
    [[nodiscard]] auto getNodeCount() const { return nodes_.size(); }
    [[nodiscard]] const Node& getNode(usize index) const { return nodes_.at(index); }
    [[nodiscard]] usize getNodeIdxByName(const std::string& name) const;

    [[nodiscard]] const std::vector<Animation>& getAnimations() const { return animations_; }
    [[nodiscard]] usize getAnimationIndexByName(const std::string& name) const;

    void evaluate(usize animation_index, f32 time, std::vector<mat4f>& out_node_globals) const;
    void evaluateNoInterpolation(usize animation_index, f32 time, std::vector<mat4f>& out_node_globals) const;

    [[nodiscard]] const auto& getSkeleton() const { return skeleton_; }

  private:
    std::vector<Node> nodes_;
    std::vector<Animation> animations_;
    std::vector<NodeMesh> meshes_;
    Skeleton skeleton_;
};
}  // namespace mle
