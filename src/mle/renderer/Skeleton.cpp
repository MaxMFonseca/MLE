#include "Skeleton.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

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

    std::vector<mat4f> ibms;
    if (skin.inverseBindMatrices >= 0) {
        const auto& ibm_acc = gltf.getAccessor(skin.inverseBindMatrices);
        ibms = gltf.readAccessorMat4f(ibm_acc);
        MLE_ASSERT_LOG(ibms.size() == joints_.size(), "inverseBindMatrices count does not match joint count");
    } else {
        ibms.assign(joints_.size(), mat4f{1.0F});
    }

    for (usize j = 0; j < skin.joints.size(); ++j) {
        const int node_index = skin.joints[j];
        const auto& node = model.nodes[as<usize>(node_index)];

        Joint& joint = joints_[j];
        joint.node_index = node_index;
        joint.inverse_bind = ibms[j];
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

void Skeleton::buildSkinMatrices(const std::vector<mat4f>& node_globals, std::vector<mat4f>& out_skin_mats) const {
    const usize joint_count = joints_.size();
    out_skin_mats.resize(joint_count);

    for (usize j = 0; j < joint_count; ++j) {
        const Joint& joint = joints_[j];
        MLE_ASSERT_LOG(joint.node_index >= 0 && as<usize>(joint.node_index) < node_globals.size(), "Joint node_index out of range for node_globals");
        const mat4f& g = node_globals[as<usize>(joint.node_index)];
        out_skin_mats[j] = g * joint.inverse_bind;
    }
}

}  // namespace mle
