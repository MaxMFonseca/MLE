#include "Animation.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <type_traits>

#include "mle/core/Assert.h"
#include "mle/renderer/AnimationCache.h"
#include "mle/renderer/Model.h"

namespace mle {
namespace {
usize findAnimationIndexByName(const GLTF& gltf, std::string_view animation_name) {
    MLE_ASSERT_LOG(!animation_name.empty(), "Animation name cannot be empty.");

    const auto& animations = gltf.model().animations;
    usize found = max<usize>();
    for (usize i = 0; i < animations.size(); ++i) {
        const auto& name = animations[i].name;
        if (name != animation_name) {
            continue;
        }

        MLE_ASSERT_LOG(found == max<usize>(), "GLTF contains duplicate animation name '{}'. Animation clip names must be unique per file.",
                       animation_name);
        found = i;
    }

    MLE_ASSERT_LOG(found != max<usize>(), "Animation '{}' was not found in GLTF.", animation_name);
    return found;
}

mat4f makeTRS(const vec3f& t, const quat& r, const vec3f& s) {
    mat4f f_t = glm::translate(mat4f{1.0F}, t);
    mat4f f_r = glm::mat4_cast(r);
    mat4f f_s = glm::scale(mat4f{1.0F}, s);
    return f_t * f_r * f_s;
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
            const float mid = (k0.time + k1.time) * 0.5F;
            return (t < mid) ? k0.value : k1.value;
        }
    }

    return keys.back().value;
}

void evaluateImpl(const AnimationClip& clip, const Model& model, const AnimationBinding& binding, f32 time, std::vector<mat4f>& out_node_globals,
                  bool interpolate) {
    const auto& model_nodes = model.getNodes();
    const usize node_count = model_nodes.size();

    out_node_globals.resize(node_count);

    thread_local std::vector<mat4f> tls_local_mats;
    if (tls_local_mats.size() < node_count) {
        tls_local_mats.resize(node_count);
    }

    const float anim_time = (clip.getDuration() > 0.0F) ? std::fmod(time, clip.getDuration()) : time;

    for (usize nid = 0; nid < node_count; ++nid) {
        const auto& node = model_nodes[nid];
        tls_local_mats[nid] = makeTRS(node.base_translation, node.base_rotation, node.base_scale);
    }

    const auto& anim_nodes = clip.getNodes();
    const auto& node_map = binding.channel_to_node_map;
    MLE_ASSERT_LOG(anim_nodes.size() == node_map.size(), "Binding map size mismatch");

    for (usize i = 0; i < anim_nodes.size(); ++i) {
        const usize nid = node_map[i];
        if (nid == max<usize>()) {
            continue;
        }

        const auto& node_anim = anim_nodes[i];
        const auto& node = model_nodes[nid];
        vec3f t = node.base_translation;
        quat r = node.base_rotation;
        vec3f s = node.base_scale;

        if (interpolate) {
            t = sampleKeyframes<AnimationClip::KeyframeVec3, vec3f>(node_anim.translations, anim_time, t);
            r = sampleKeyframes<AnimationClip::KeyframeQuat, quat>(node_anim.rotations, anim_time, r);
            s = sampleKeyframes<AnimationClip::KeyframeVec3, vec3f>(node_anim.scales, anim_time, s);
        } else {
            t = sampleNearestKeyframeNoInterp<AnimationClip::KeyframeVec3, vec3f>(node_anim.translations, anim_time, t);
            r = sampleNearestKeyframeNoInterp<AnimationClip::KeyframeQuat, quat>(node_anim.rotations, anim_time, r);
            s = sampleNearestKeyframeNoInterp<AnimationClip::KeyframeVec3, vec3f>(node_anim.scales, anim_time, s);
        }

        tls_local_mats[nid] = makeTRS(t, r, s);
    }

    for (usize nid : model.getEvaluationOrder()) {
        const auto& node = model_nodes[nid];
        if (node.parent >= 0) {
            out_node_globals[nid] = out_node_globals[as<usize>(node.parent)] * tls_local_mats[nid];
        } else {
            out_node_globals[nid] = tls_local_mats[nid];
        }
    }
}
}  // namespace

void AnimationClip::loadFromGLTF(const GLTF& gltf, std::string_view animation_name) {
    loadFromGLTF(gltf, findAnimationIndexByName(gltf, animation_name));
}

void AnimationClip::loadFromGLTF(const GLTF& gltf, usize animation_index) {
    const auto& model = gltf.model();
    MLE_ASSERT_LOG(animation_index < model.animations.size(), "animation_index out of range");

    const auto& anim_src = model.animations[animation_index];

    name_ = anim_src.name;
    duration_ = 0.0F;
    nodes_.clear();

    std::vector<int> node_to_anim(model.nodes.size(), -1);

    for (const auto& channel : anim_src.channels) {
        const int sampler_idx = channel.sampler;
        MLE_ASSERT_LOG(sampler_idx >= 0 && sampler_idx < as<int>(anim_src.samplers.size()), "Invalid sampler index");

        const auto& sampler = anim_src.samplers[as<usize>(sampler_idx)];
        MLE_ASSERT_LOG(sampler.interpolation == "LINEAR" || sampler.interpolation == "STEP", "Only LINEAR/STEP interpolation supported for node animations");

        const int target_node = channel.target_node;
        if (target_node < 0 || target_node >= as<int>(model.nodes.size())) {
            continue;
        }

        const auto& time_acc = gltf.getAccessor(sampler.input);
        std::vector<f32> times = gltf.readAccessorFloat(time_acc);
        if (!times.empty()) {
            duration_ = std::max(duration_, times.back());
        }

        int node_anim_idx = node_to_anim[as<usize>(target_node)];
        if (node_anim_idx < 0) {
            node_anim_idx = as<int>(nodes_.size());
            node_to_anim[as<usize>(target_node)] = node_anim_idx;

            NodeAnim node_anim{};
            node_anim.target_name = model.nodes[as<usize>(target_node)].name;
            nodes_.push_back(std::move(node_anim));
        }

        NodeAnim& node_anim = nodes_[as<usize>(node_anim_idx)];
        const std::string& path = channel.target_path;

        if (path == "translation") {
            const auto& out_acc = gltf.getAccessor(sampler.output);
            std::vector<vec3f> values = gltf.readAccessorVec3f(out_acc);
            MLE_ASSERT_LOG(values.size() == times.size(), "translation: times/values size mismatch");

            node_anim.translations.reserve(node_anim.translations.size() + times.size());
            for (usize i = 0; i < times.size(); ++i) {
                node_anim.translations.push_back(KeyframeVec3{.time = times[i], .value = values[i]});
            }
        } else if (path == "rotation") {
            const auto& out_acc = gltf.getAccessor(sampler.output);
            std::vector<quat> values = gltf.readAccessorQuat(out_acc);
            MLE_ASSERT_LOG(values.size() == times.size(), "rotation: times/values size mismatch");

            node_anim.rotations.reserve(node_anim.rotations.size() + times.size());
            for (usize i = 0; i < times.size(); ++i) {
                node_anim.rotations.push_back(KeyframeQuat{.time = times[i], .value = glm::normalize(values[i])});
            }
        } else if (path == "scale") {
            const auto& out_acc = gltf.getAccessor(sampler.output);
            std::vector<vec3f> values = gltf.readAccessorVec3f(out_acc);
            MLE_ASSERT_LOG(values.size() == times.size(), "scale: times/values size mismatch");

            node_anim.scales.reserve(node_anim.scales.size() + times.size());
            for (usize i = 0; i < times.size(); ++i) {
                node_anim.scales.push_back(KeyframeVec3{.time = times[i], .value = values[i]});
            }
        } else {
            MLE_NOOP;
        }
    }
}

void AnimationClip::evaluate(const Model& model, const AnimationBinding& binding, f32 time, std::vector<mat4f>& out_node_globals) const {
    evaluateImpl(*this, model, binding, time, out_node_globals, true);
}

void AnimationClip::evaluateNoInterpolation(const Model& model, const AnimationBinding& binding, f32 time, std::vector<mat4f>& out_node_globals) const {
    evaluateImpl(*this, model, binding, time, out_node_globals, false);
}
}  // namespace mle
