#include "Model.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "mle/core/Assert.h"
#include "mle/renderer/Buffer.h"

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

mat4f makeTRS(const vec3f& t, const quat& r, const vec3f& s) {
    mat4f f_t = glm::translate(mat4f{1.0F}, t);
    mat4f f_r = glm::mat4_cast(r);
    mat4f f_s = glm::scale(mat4f{1.0F}, s);
    return f_t * f_r * f_s;
}

// NOLINTNEXTLINE(misc-no-recursion)  know
void computeNodeGlobalRecursive(usize node_index, const std::vector<Model::Node>& nodes, const std::vector<mat4f>& local_mats, std::vector<mat4f>& global_mats,
                                std::vector<bool>& visited) {
    if (visited[node_index]) {
        return;
    }

    const auto& node = nodes[node_index];
    if (node.parent >= 0) {
        const auto parent_idx = as<usize>(node.parent);
        computeNodeGlobalRecursive(parent_idx, nodes, local_mats, global_mats, visited);
        global_mats[node_index] = global_mats[parent_idx] * local_mats[node_index];
    } else {
        global_mats[node_index] = local_mats[node_index];
    }

    visited[node_index] = true;
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

template <typename KeyframeT, typename ValueT>
ValueT sampleNearestKeyframeNoInterp(const std::vector<KeyframeT>& keys, f32 t, ValueT identity) {
    if (keys.empty()) {
        return identity;
    }

    // Clamp range
    if (t <= keys.front().time) {
        return keys.front().value;
    }
    if (t >= keys.back().time) {
        return keys.back().value;
    }

    // Find nearest neighbor between k0 and k1
    for (usize i = 0; i + 1 < keys.size(); ++i) {
        const auto& k0 = keys[i];
        const auto& k1 = keys[i + 1];
        if (t >= k0.time && t <= k1.time) {
            const float mid = (k0.time + k1.time) * 0.5F;
            return (t < mid) ? k0.value : k1.value;
        }
    }

    return keys.back().value;
}

}  // namespace

void Model::init(const GLTF& gltf) {
    const auto& model = gltf.model();

    nodes_.clear();
    animations_.clear();
    meshes_.clear();

    const usize node_count = model.nodes.size();
    nodes_.resize(node_count);

    std::vector<int> node_parent;
    buildNodeParents(model, node_parent);

    for (usize nid = 0; nid < node_count; ++nid) {
        const tinygltf::Node& src_node = model.nodes[nid];

        Node& node = nodes_[nid];
        node.parent = (nid < node_parent.size()) ? node_parent[nid] : -1;
        node.name = src_node.name;

        auto [t, r, s] = getNodeLocalTRS(src_node);
        node.base_translation = t;
        node.base_rotation = r;
        node.base_scale = s;

        if (src_node.mesh >= 0) {
            MLE_ASSERT_LOG(src_node.mesh < as<int>(model.meshes.size()), "Invalid mesh index in node");
            NodeMesh nm{};
            nm.node_index = nid;
            nm.mesh.load(gltf, as<usize>(src_node.mesh));
            meshes_.push_back(std::move(nm));
        }
    }

    animations_.reserve(model.animations.size());

    for (const auto& anim_src : model.animations) {
        Animation anim{};
        anim.name = anim_src.name;
        anim.duration = 0.0F;
        anim.nodes.resize(node_count);  // one NodeAnim per node

        for (const auto& channel : anim_src.channels) {
            const int sampler_idx = channel.sampler;
            MLE_ASSERT_LOG(sampler_idx >= 0 && sampler_idx < as<int>(anim_src.samplers.size()), "Invalid sampler index");

            const auto& sampler = anim_src.samplers[as<usize>(sampler_idx)];

            MLE_ASSERT_LOG(sampler.interpolation == "LINEAR" || sampler.interpolation == "STEP",
                           "Only LINEAR/STEP interpolation supported for node animations");

            const int target_node = channel.target_node;
            if (target_node < 0 || target_node >= as<int>(model.nodes.size())) {
                continue;
            }

            const auto& time_acc = gltf.getAccessor(sampler.input);
            std::vector<f32> times = gltf.readAccessorFloat(time_acc);
            if (!times.empty()) {
                anim.duration = std::max(anim.duration, times.back());
            }

            NodeAnim& node_anim = anim.nodes[as<usize>(target_node)];
            const std::string& path = channel.target_path;

            if (path == "translation") {
                const auto& out_acc = gltf.getAccessor(sampler.output);
                std::vector<vec3f> values = gltf.readAccessorVec3f(out_acc);
                MLE_ASSERT_LOG(values.size() == times.size(), "translation: times/values size mismatch");

                const usize count = times.size();
                node_anim.translations.reserve(node_anim.translations.size() + count);
                for (usize i = 0; i < count; ++i) {
                    KeyframeVec3 kf{};
                    kf.time = times[i];
                    kf.value = values[i];
                    node_anim.translations.push_back(kf);
                }
            } else if (path == "rotation") {
                const auto& out_acc = gltf.getAccessor(sampler.output);
                std::vector<quat> values = gltf.readAccessorQuat(out_acc);
                MLE_ASSERT_LOG(values.size() == times.size(), "rotation: times/values size mismatch");

                const usize count = times.size();
                node_anim.rotations.reserve(node_anim.rotations.size() + count);
                for (usize i = 0; i < count; ++i) {
                    KeyframeQuat kf{};
                    kf.time = times[i];
                    kf.value = glm::normalize(values[i]);
                    node_anim.rotations.push_back(kf);
                }
            } else if (path == "scale") {
                const auto& out_acc = gltf.getAccessor(sampler.output);
                std::vector<vec3f> values = gltf.readAccessorVec3f(out_acc);
                MLE_ASSERT_LOG(values.size() == times.size(), "scale: times/values size mismatch");

                const usize count = times.size();
                node_anim.scales.reserve(node_anim.scales.size() + count);
                for (usize i = 0; i < count; ++i) {
                    KeyframeVec3 kf{};
                    kf.time = times[i];
                    kf.value = values[i];
                    node_anim.scales.push_back(kf);
                }
            } else {
                MLE_NOOP;  // weights/morph etc. ignored here
            }
        }

        animations_.push_back(std::move(anim));
    }

    if (!model.skins.empty()) {
        skeleton_.loadFromGLTF(gltf);
    }
}

void Model::evaluate(usize animation_index, f32 time, std::vector<mat4f>& out_node_globals) const {
    MLE_ASSERT_LOG(animation_index < animations_.size(), "animation_index out of range");

    const Animation& anim = animations_[animation_index];
    const usize node_count = nodes_.size();

    out_node_globals.resize(node_count);

    std::vector<mat4f> local_mats(node_count);
    std::vector<mat4f> global_mats(node_count);
    std::vector<bool> visited(node_count, false);

    const float anim_time = (anim.duration > 0.0F) ? std::fmod(time, anim.duration) : time;

    for (usize nid = 0; nid < node_count; ++nid) {
        const Node& node = nodes_[nid];
        const NodeAnim& na = anim.nodes[nid];

        vec3f t = node.base_translation;
        quat r = node.base_rotation;
        vec3f s = node.base_scale;

        if (!na.translations.empty()) {
            t = sampleKeyframes<Model::KeyframeVec3, vec3f>(na.translations, anim_time, t);
        }
        if (!na.rotations.empty()) {
            r = sampleKeyframes<Model::KeyframeQuat, quat>(na.rotations, anim_time, r);
        }
        if (!na.scales.empty()) {
            s = sampleKeyframes<Model::KeyframeVec3, vec3f>(na.scales, anim_time, s);
        }

        local_mats[nid] = makeTRS(t, r, s);
    }

    for (usize nid = 0; nid < node_count; ++nid) {
        if (!visited[nid]) {
            computeNodeGlobalRecursive(nid, nodes_, local_mats, global_mats, visited);
        }
    }

    out_node_globals = global_mats;
}

void Model::evaluateNoInterpolation(usize animation_index, f32 time, std::vector<mat4f>& out_node_globals) const {
    MLE_ASSERT_LOG(animation_index < animations_.size(), "animation_index out of range");
    const Animation& anim = animations_[animation_index];

    const usize node_count = nodes_.size();
    out_node_globals.resize(node_count);

    std::vector<mat4f> local_mats(node_count);
    std::vector<mat4f> global_mats(node_count);
    std::vector<bool> visited(node_count, false);

    const float anim_time = (anim.duration > 0.0F) ? std::fmod(time, anim.duration) : time;

    for (usize nid = 0; nid < node_count; ++nid) {
        const Node& node = nodes_[nid];
        const NodeAnim& na = anim.nodes[nid];

        vec3f t = node.base_translation;
        quat r = node.base_rotation;
        vec3f s = node.base_scale;

        if (!na.translations.empty()) {
            t = sampleNearestKeyframeNoInterp<Model::KeyframeVec3, vec3f>(na.translations, anim_time, t);
        }
        if (!na.rotations.empty()) {
            r = sampleNearestKeyframeNoInterp<Model::KeyframeQuat, quat>(na.rotations, anim_time, r);
        }
        if (!na.scales.empty()) {
            s = sampleNearestKeyframeNoInterp<Model::KeyframeVec3, vec3f>(na.scales, anim_time, s);
        }

        local_mats[nid] = makeTRS(t, r, s);
    }

    for (usize nid = 0; nid < node_count; ++nid) {
        if (!visited[nid]) {
            computeNodeGlobalRecursive(nid, nodes_, local_mats, global_mats, visited);
        }
    }

    out_node_globals = global_mats;
}

usize Model::getNodeIdxByName(const std::string& name) const {
    for (usize i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i].name == name) {
            return i;
        }
    }
    MLE_E("Node '{}' not found in Model", name);
    return max<usize>();
}

usize Model::getAnimationIndexByName(const std::string& name) const {
    for (usize i = 0; i < animations_.size(); ++i) {
        if (animations_[as<usize>(i)].name == name) {
            return i;
        }
    }
    MLE_E("Animation '{}' not found in NodeAnimations", name);
    return max<usize>();
}

}  // namespace mle
