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
struct AnimationBinding {
    std::vector<usize> channel_to_node_map;
};

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

    const AnimationBinding& getBinding(ModelRef model, AnimationClipRef clip);

  private:
    friend class Renderer;
    AnimationCache() = default;
    void init() {};
    void shutdown();

    static void validateAnimationNames(std::string_view source_name, const GLTF& gltf);

  private:
    std::unordered_map<entt::id_type, AnimationClipHnd> animations_;

    struct BindingKey {
        ModelRef model;
        AnimationClipRef clip;

        bool operator==(const BindingKey& other) const { return model == other.model && clip == other.clip; }
    };

    struct BindingKeyHash {
        std::size_t operator()(const BindingKey& k) const {
            return std::hash<const void*>{}(k.model) ^ (std::hash<const void*>{}(k.clip) << 1);
        }
    };

    std::unordered_map<BindingKey, AnimationBinding, BindingKeyHash> bindings_;
};
}  // namespace mle
