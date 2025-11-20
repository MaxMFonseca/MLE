#pragma once

#include <tiny_gltf.h>

#include <string>
#include <vector>

#include "mle/math/Types.h"
#include "mle/renderer/GLTF.h"

namespace mle {

class Skeleton {
  public:
    struct Joint {
        int parent = -1;
        int node_index = -1;

        mat4f inverse_bind;
        vec3f local_translation;
        quat local_rotation;
        vec3f local_scale;

        std::string name;
    };

    struct KeyframeVec3 {
        f32 time = 0.0F;
        vec3f value{};
    };

    struct KeyframeQuat {
        f32 time = 0.0F;
        quat value{};
    };

    struct JointAnim {
        std::vector<KeyframeVec3> translations;
        std::vector<KeyframeQuat> rotations;
        std::vector<KeyframeVec3> scales;
    };

    struct Animation {
        std::string name;
        f32 duration = 0.0F;
        std::vector<JointAnim> joints;
    };

  public:
    void load(const GLTF& gltf);

    [[nodiscard]] const std::vector<Joint>& getJoints() const { return joints_; }
    [[nodiscard]] const std::vector<Animation>& getAnimations() const { return animations_; }
    [[nodiscard]] u32 getAnimationIndexByName(const std::string& name) const;
    void evaluate(u32 animation_index, f32 time, std::vector<mat4f>& out_skin_mats) const;

  private:
    std::vector<Joint> joints_;
    std::vector<Animation> animations_;
};
}  // namespace mle
