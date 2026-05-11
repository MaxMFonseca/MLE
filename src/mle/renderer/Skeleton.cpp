#include "Skeleton.h"

#include "mle/core/Assert.h"

namespace mle {
namespace {
void buildNodeParents(const tinygltf::Model& model, std::vector<int>& out_node_parent) {
    out_node_parent.assign(model.nodes.size(), -1);
    for (int nid = 0; nid < as<int>(model.nodes.size()); ++nid) {
        const auto& node = model.nodes[as<usize>(nid)];
        for (int child : node.children) {
            MLE_ASSERT_LOG(child >= 0 && child < as<int>(model.nodes.size()), "Invalid child node index");
            out_node_parent[as<usize>(child)] = nid;
        }
    }
}
}  // namespace

void Skeleton::loadFromGLTF(const GLTF& gltf) {
    const auto& model = gltf.model();

    joints_.clear();

    MLE_ASSERT_LOG(model.skins.size() == 1, "Only single skin supported");

    const auto& skin = model.skins.front();
    MLE_ASSERT_LOG(!skin.joints.empty(), "Skin has no joints");

    joints_.resize(skin.joints.size());

    std::vector<int> node_to_joint(model.nodes.size(), -1);
    for (usize j = 0; j < skin.joints.size(); ++j) {
        const int node_index = skin.joints[j];
        MLE_ASSERT_LOG(node_index >= 0 && node_index < as<int>(model.nodes.size()), "Invalid joint node index");
        node_to_joint[as<usize>(node_index)] = as<int>(j);
    }

    std::vector<int> node_parent;
    buildNodeParents(model, node_parent);

    for (usize j = 0; j < skin.joints.size(); ++j) {
        const int node_index = skin.joints[j];
        const auto& node = model.nodes[as<usize>(node_index)];

        Joint& joint = joints_[j];
        joint.name = node.name;

        int parent_joint = -1;
        if (node_index >= 0 && as<usize>(node_index) < node_parent.size()) {
            const int parent_node = node_parent[as<usize>(node_index)];
            if (parent_node >= 0 && as<usize>(parent_node) < node_to_joint.size()) {
                parent_joint = node_to_joint[as<usize>(parent_node)];
            }
        }
        joint.parent = parent_joint;
    }
}
}  // namespace mle
