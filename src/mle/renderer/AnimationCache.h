#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "mle/renderer/Animation.h"
#include "mle/renderer/Types.h"

namespace mle {
class AnimationCache {
  public:
    MLE_NO_COPY_MOVE(AnimationCache);

    ~AnimationCache() = default;

    std::vector<AnimationClipRef> addAnimations(const std::string& animation_file);
    AnimationClipRef addAnimation(const std::string& animation_file, usize animation_index);
    AnimationClipRef getAnimation(const std::string& animation_name);

  private:
    friend class Renderer;
    AnimationCache() = default;
    void init() {};
    void shutdown() {};

    [[nodiscard]] static std::string makeAnimationName(const std::string& animation_file, const AnimationClip& clip, usize animation_index);

  private:
    std::unordered_map<std::string, AnimationClipHnd> animations_;
};
}  // namespace mle
