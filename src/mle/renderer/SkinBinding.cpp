#include "SkinBinding.h"

#include "mle/core/Assert.h"

namespace mle {
void SkinBinding::loadFromGLTF(const GLTF& gltf, usize skin_index) {
    const auto& model = gltf.model();

    joints_.clear();

    MLE_ASSERT_LOG(skin_index < model.skins.size(), "skin_index out of range");

    const auto& skin = model.skins[skin_index];
    MLE_ASSERT_LOG(!skin.joints.empty(), "Skin has no joints");

    name_ = skin.name;
    joints_.resize(skin.joints.size());

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
        MLE_ASSERT_LOG(node_index >= 0 && node_index < as<int>(model.nodes.size()), "Invalid joint node index");

        JointBinding& joint = joints_[j];
        joint.node_index = node_index;
        joint.inverse_bind = ibms[j];
        joint.name = model.nodes[as<usize>(node_index)].name;
    }
}

void SkinBinding::buildSkinMatrices(const std::vector<mat4f>& node_globals, std::vector<mat4f>& out_skin_mats) const {
    const usize joint_count = joints_.size();
    out_skin_mats.resize(joint_count);

    for (usize j = 0; j < joint_count; ++j) {
        const JointBinding& joint = joints_[j];
        MLE_ASSERT_LOG(joint.node_index >= 0 && as<usize>(joint.node_index) < node_globals.size(), "Joint node_index out of range for node_globals");
        const mat4f& g = node_globals[as<usize>(joint.node_index)];
        out_skin_mats[j] = g * joint.inverse_bind;
    }
}
}  // namespace mle
