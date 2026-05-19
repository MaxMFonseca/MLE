#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "mle/core/Consts.h"
#include "mle/renderer/Animation.h"
#include "mle/renderer/GLTF.h"
#include "mle/renderer/Types.h"
#include "mle/utils/ECS.h"

namespace mle {
class AnimationCache {
  public:
    MLE_NO_COPY_MOVE(AnimationCache);

    ~AnimationCache() = default;

    [[nodiscard]] static entt::id_type makeAnimationId(std::string_view source_name, std::string_view animation_name);

    std::vector<AnimationClipRef> addAnimations(std::string_view source_name, const GLTF& gltf);
    std::vector<AnimationClipRef> addAnimations(const std::string& animation_file, const std::string& resource_dir = ResPath::MODELS);
    AnimationClipRef addAnimation(std::string_view source_name, const GLTF& gltf, std::string_view animation_name);
    AnimationClipRef get(entt::id_type id);
    [[nodiscard]] bool contains(entt::id_type id) const;

  private:
    friend class Renderer;
    AnimationCache() = default;
    void init() {};
    void shutdown();

    static void validateAnimationNames(std::string_view source_name, const GLTF& gltf);

  private:
    std::unordered_map<entt::id_type, AnimationClipHnd> animations_;
};
}  // namespace mle
