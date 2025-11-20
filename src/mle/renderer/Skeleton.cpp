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

std::tuple<vec3f, quat, vec3f> getNodeLocalTRS(const tinygltf::Node& node) {
    vec3f t{};
    quat r{};
    vec3f s{};

    if (node.matrix.size() == 16) {
        mat4f m{};
        for (int i = 0; i < 16; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
            glm::value_ptr(m)[i] = as<float>(node.matrix[as<usize>(i)]);
        }

        vec3f skew;
        vec4f perspective;

        bool ok = glm::decompose(m, s, r, t, skew, perspective);
        MLE_ASSERT_LOG(ok, "Failed to decompose node.matrix");
        r = glm::normalize(r);
    }

    if (!node.translation.empty()) {
        t = vec3f{
            as<f32>(node.translation[0]),
            as<f32>(node.translation[1]),
            as<f32>(node.translation[2]),
        };
    } else {
        t = vec3f{0.0F, 0.0F, 0.0F};
    }

    if (!node.rotation.empty()) {
        r = quat{
            as<f32>(node.rotation[3]),
            as<f32>(node.rotation[0]),
            as<f32>(node.rotation[1]),
            as<f32>(node.rotation[2]),
        };
        r = glm::normalize(r);
    } else {
        r = quat{1.0F, 0.0F, 0.0F, 0.0F};
    }

    if (!node.scale.empty()) {
        s = vec3f{
            as<f32>(node.scale[0]),
            as<f32>(node.scale[1]),
            as<f32>(node.scale[2]),
        };
    } else {
        s = vec3f{1.0F, 1.0F, 1.0F};
    }

    return std::make_tuple(t, r, s);
}

template <typename KeyframeT, typename ValueT>
ValueT sampleKeyframes(const std::vector<KeyframeT>& keys, f32 t, ValueT identity) {
    if (keys.empty()) {
        return identity;
    }
    if (t <= keys.front().time) {
        return keys.front().value;
    }
    if (t >= keys.back().time) {
        return keys.back().value;
    }

    for (usize i = 0; i + 1 < keys.size(); ++i) {
        const auto& k0 = keys[i];
        const auto& k1 = keys[i + 1];
        if (t >= k0.time && t <= k1.time) {
            const float dt = k1.time - k0.time;
            const float alpha = (dt > 0.0F) ? ((t - k0.time) / dt) : 0.0F;
            if constexpr (std::is_same_v<ValueT, vec3f>) {
                return glm::mix(k0.value, k1.value, alpha);
            } else {
                // quaternion
                return glm::slerp(k0.value, k1.value, alpha);
            }
        }
    }

    return keys.back().value;
}

}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity) fine
void Skeleton::load(const GLTF& gltf) {
    const auto& model = gltf.model();

    joints_.clear();
    animations_.clear();

    MLE_ASSERT_LOG(model.skins.size() == 1, "Only single skin supported");

    const auto& skin = model.skins.front();

    MLE_ASSERT_LOG(!skin.joints.empty(), "Skin has no joints");

    std::vector<int> node_to_joint(model.nodes.size(), -1);
    joints_.resize(skin.joints.size());

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
        MLE_ASSERT_LOG(node_index >= 0 && node_index < as<int>(model.nodes.size()), "Invalid joint node index");

        node_to_joint[as<usize>(node_index)] = as<int>(j);

        const auto& node = model.nodes[as<usize>(node_index)];

        Joint& joint = joints_[j];
        joint.node_index = node_index;
        joint.inverse_bind = ibms[j];
        joint.name = node.name;

        int parent_node = node_parent[as<usize>(node_index)];
        int parent_joint = -1;
        if (parent_node >= 0) {
            if (parent_node < as<int>(node_to_joint.size())) {
                parent_joint = node_to_joint[as<usize>(parent_node)];
            }
        }
        joint.parent = parent_joint;

        auto trs = getNodeLocalTRS(node);
        joint.local_translation = std::get<0>(trs);
        joint.local_rotation = std::get<1>(trs);
        joint.local_scale = std::get<2>(trs);
    }

    for (auto& joint : joints_) {
        if (joint.parent == -1 && joint.node_index >= 0) {
            const int parent_node = node_parent[as<usize>(joint.node_index)];
            if (parent_node >= 0) {
                const int pj = node_to_joint[as<usize>(parent_node)];
                if (pj >= 0) {
                    joint.parent = pj;
                }
            }
        }
    }

    animations_.reserve(model.animations.size());

    for (const auto& anim_src : model.animations) {
        Animation anim{};
        anim.name = anim_src.name;
        anim.duration = 0.0F;
        anim.joints.resize(joints_.size());

        for (const auto& channel : anim_src.channels) {
            const int sampler_idx = channel.sampler;
            MLE_ASSERT_LOG(sampler_idx >= 0 && sampler_idx < as<int>(anim_src.samplers.size()), "Invalid sampler index");

            const auto& sampler = anim_src.samplers[as<usize>(sampler_idx)];
            MLE_ASSERT_LOG(sampler.interpolation == "LINEAR" || sampler.interpolation == "STEP", "Only LINEAR/STEP interpolation supported");

            const int target_node = channel.target_node;
            if (target_node < 0 || target_node >= as<int>(model.nodes.size())) {
                continue;
            }

            const int joint_index = node_to_joint[as<usize>(target_node)];
            if (joint_index < 0) {
                continue;
            }

            const auto& time_acc = gltf.getAccessor(sampler.input);
            auto times = gltf.readAccessorFloat(time_acc);
            if (!times.empty()) {
                anim.duration = std::max(anim.duration, times.back());
            }

            auto& joint_anim = anim.joints[as<usize>(joint_index)];

            const std::string& path = channel.target_path;

            if (path == "translation") {
                const auto& out_acc = gltf.getAccessor(sampler.output);
                auto values = gltf.readAccessorVec3f(out_acc);
                MLE_ASSERT_LOG(values.size() == times.size(), "translation: times/values size mismatch");

                const usize count = times.size();
                joint_anim.translations.reserve(joint_anim.translations.size() + count);
                for (usize i = 0; i < count; ++i) {
                    KeyframeVec3 kf{};
                    kf.time = times[i];
                    kf.value = values[i];
                    joint_anim.translations.push_back(kf);
                }
            } else if (path == "rotation") {
                const auto& out_acc = gltf.getAccessor(sampler.output);
                auto values = gltf.readAccessorQuat(out_acc);
                MLE_ASSERT_LOG(values.size() == times.size(), "rotation: times/values size mismatch");

                const usize count = times.size();
                joint_anim.rotations.reserve(joint_anim.rotations.size() + count);
                for (usize i = 0; i < count; ++i) {
                    KeyframeQuat kf{};
                    kf.time = times[i];
                    kf.value = values[i];
                    joint_anim.rotations.push_back(kf);
                }
            } else if (path == "scale") {
                const auto& out_acc = gltf.getAccessor(sampler.output);
                auto values = gltf.readAccessorVec3f(out_acc);
                MLE_ASSERT_LOG(values.size() == times.size(), "scale: times/values size mismatch");

                const usize count = times.size();
                joint_anim.scales.reserve(joint_anim.scales.size() + count);
                for (usize i = 0; i < count; ++i) {
                    KeyframeVec3 kf{};
                    kf.time = times[i];
                    kf.value = values[i];
                    joint_anim.scales.push_back(kf);
                }
            } else {
                MLE_NOOP;
            }
        }

        animations_.push_back(std::move(anim));
    }
}

u32 Skeleton::getAnimationIndexByName(const std::string& name) const {
    for (u32 i = 0; i < as<u32>(animations_.size()); ++i) {
        if (animations_[as<usize>(i)].name == name) {
            return i;
        }
    }
    MLE_E("Animation '{}' not found in Skeleton", name);
    return max<u32>();
}

void Skeleton::evaluate(u32 animation_index, f32 time, std::vector<mat4f>& out_skin_mats) const {
    MLE_ASSERT_LOG(animation_index < animations_.size(), "animation_index out of range");
    const Animation& anim = animations_[animation_index];

    const usize joint_count = joints_.size();
    out_skin_mats.resize(joint_count);

    std::vector<mat4f> local_mats(joint_count);
    std::vector<mat4f> global_mats(joint_count);

    const float anim_time = (anim.duration > 0.0F) ? std::fmod(time, anim.duration) : time;

    for (usize j = 0; j < joint_count; ++j) {
        const Joint& joint = joints_[j];
        const JointAnim& ja = anim.joints[j];

        vec3f t = joint.local_translation;
        quat r = joint.local_rotation;
        vec3f s = joint.local_scale;

        if (!ja.translations.empty()) {
            t = sampleKeyframes<KeyframeVec3, vec3f>(ja.translations, anim_time, t);
        }
        if (!ja.rotations.empty()) {
            r = sampleKeyframes<KeyframeQuat, quat>(ja.rotations, anim_time, r);
        }
        if (!ja.scales.empty()) {
            s = sampleKeyframes<KeyframeVec3, vec3f>(ja.scales, anim_time, s);
        }

        mat4f f_t = glm::translate(mat4f{1.0F}, t);
        mat4f f_r = glm::mat4_cast(r);
        mat4f f_s = glm::scale(mat4f{1.0F}, s);

        local_mats[j] = f_t * f_r * f_s;
    }

    for (usize j = 0; j < joint_count; ++j) {
        const Joint& joint = joints_[j];
        if (joint.parent >= 0) {
            global_mats[j] = global_mats[as<usize>(joint.parent)] * local_mats[j];
        } else {
            global_mats[j] = local_mats[j];
        }
    }

    for (usize j = 0; j < joint_count; ++j) {
        out_skin_mats[j] = global_mats[j] * joints_[j].inverse_bind;
    }
}
}  // namespace mle
