#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "mle/math/Types.h"
#include "mle/renderer/GLTF.h"

namespace mle {
class AnimationCache;
class Model;
struct AnimationBinding;

class AnimationClip {
  public:
    struct KeyframeVec3 {
        f32 time = 0.0F;
        vec3f value{};
    };

    struct KeyframeQuat {
        f32 time = 0.0F;
        quat value{};
    };

    struct NodeAnim {
        std::string target_name;
        std::vector<KeyframeVec3> translations;
        std::vector<KeyframeQuat> rotations;
        std::vector<KeyframeVec3> scales;
    };

  public:
    void loadFromGLTF(const GLTF& gltf, std::string_view animation_name);

    [[nodiscard]] const std::string& getName() const { return name_; }
    [[nodiscard]] f32 getDuration() const { return duration_; }
    [[nodiscard]] const std::vector<NodeAnim>& getNodes() const { return nodes_; }

    void evaluate(const Model& model, const AnimationBinding& binding, f32 time, std::span<mat4f> out_node_globals) const;
    void evaluateNoInterpolation(const Model& model, const AnimationBinding& binding, f32 time, std::span<mat4f> out_node_globals) const;

  private:
    friend class AnimationCache;

    void loadFromGLTF(const GLTF& gltf, usize animation_index);

    std::string name_;
    f32 duration_ = 0.0F;
    std::vector<NodeAnim> nodes_;
};
}  // namespace mle
